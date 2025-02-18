#include "httpTask.hpp"

static constexpr char *TEXT_HTML = "text/html";
static constexpr char *TEXT_PLAIN = "text/plain";

static constexpr char *CONTENT_ENCODING = "Content-Encoding";
static constexpr char *GZIP = "gzip";

static constexpr char *IF_MODIFIED_SINCE = "If-Modified-Since";
static constexpr char *IF_NONE_MATCH = "If-None-Match";

static char contentCreationTime[30];

static inline bool samePageIsCached(PsychicRequest *request, const char *date, const char *etag)
{
    bool modifiedSince = request->hasHeader(IF_MODIFIED_SINCE) && request->header(IF_MODIFIED_SINCE).equals(date);
    bool noneMatch = request->hasHeader(IF_NONE_MATCH) && request->header(IF_NONE_MATCH).equals(etag);

    return modifiedSince || noneMatch;
}

static void addStaticContentHeaders(PsychicResponse &response, const char *date, const char *etag)
{
    response.addHeader("Last-Modified", date);
    response.addHeader("Cache-Control", "public, max-age=31536000");
    response.addHeader("ETag", etag);
}

static char etagValue[16];
static void generateETag(const char *date)
{
    uint32_t hash = 0;
    for (const char *p = date; *p; ++p)
        hash = (hash * 31) + *p; // Simple hash function

    snprintf(etagValue, sizeof(etagValue), "\"%" PRIX32 "\"", hash);
}

bool loadMoonSettings(String &result)
{
    if (!spiMutex)
    {
        result = "SPI mutex not initialized";
        return false;
    }

    ScopedMutex scopedMutex(spiMutex);

    if (!scopedMutex.acquired())
    {
        result = "Mutex timeout";
        return false;
    }

    if (!SD.begin(SDCARD_SS))
    {
        result = "Failed to initialize SD card";
        return false;
    }

    File file = SD.open(MOON_SETTINGS_FILE, FILE_READ);
    if (!file)
    {
        SD.end();
        result = "Failed to open ";
        result.concat(MOON_SETTINGS_FILE);
        result.concat(" for reading");
        return false;
    }

    std::array<float, NUMBER_OF_CHANNELS> tempLevels;

    for (int i = 0; i < NUMBER_OF_CHANNELS; ++i)
    {
        String header = file.readStringUntil('\n');
        String value = file.readStringUntil('\n');

        if (header.isEmpty() || value.isEmpty())
        {
            result = "Unexpected EOF while reading channel ";
            result.concat(i);
            return false;
        }

        int readIndex = -1;
        if (sscanf(header.c_str(), "[%d]", &readIndex) != 1 || readIndex != i)
        {
            result = "Invalid header format or index mismatch: expected [";
            result.concat(i);
            result.concat("], got '");
            result.concat(header);
            result.concat("'");
            return false;
        }

        float level = value.toFloat();
        if (!isfinite(level) || level < 0.0f || level > 100.0f)
        {
            result = "Invalid float value for channel " + String(i) + " '" + String(value) + "' (must be between 0 and 100)";
            return false;
        }

        tempLevels[i] = level;
    }

    file.close();
    SD.end();

    {
        std::lock_guard<std::mutex> lock(channelMutex);
        std::copy(tempLevels.begin(), tempLevels.end(), fullMoonLevel);
    }

    log_i("Loaded moon settings from %s", MOON_SETTINGS_FILE);
    result = "Moon settings processed";
    return true;
}

bool saveMoonSettings(String &result)
{
    if (!spiMutex)
    {
        result = "SPI mutex not initialized";
        log_e("%s", result.c_str());
        return false;
    }

    ScopedMutex scopedMutex(spiMutex);

    if (!scopedMutex.acquired())
    {
        result = "Mutex timeout";
        log_w("%s", result.c_str());
        return false;
    }

    if (!SD.begin(SDCARD_SS))
    {
        result = "Failed to initialize SD card";
        log_e("%s", result.c_str());
        return false;
    }

    File file = SD.open(MOON_SETTINGS_FILE, FILE_WRITE);
    if (!file)
    {
        SD.end();
        result = "Failed to open ";
        result.concat(MOON_SETTINGS_FILE);
        result.concat(" for writing");
        log_e("%s", result.c_str());
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(channelMutex);
        for (int i = 0; i < NUMBER_OF_CHANNELS; ++i)
        {
            file.printf("[%d]\n", i);
            file.printf("%.6f\n", fullMoonLevel[i]);
        }
    }

    file.close();
    SD.end();

    result = "Saved moon settings to ";
    result.concat(MOON_SETTINGS_FILE);
    log_i("%s", result.c_str());
    return true;
}

static void setupWebsocketHandler(PsychicWebSocketHandler &websocketHandler)
{
#define SHOW_WS_CONNECTIONS 0
#if SHOW_WS_CONNECTIONS
    websocketHandler.onOpen(
        [](PsychicWebSocketClient *client)
        {
            log_i("[socket] connection #%u connected from %s", client->socket(), client->remoteIP().toString());
        });

    websocketHandler.onClose(
        [](PsychicWebSocketClient *client)
        {
            log_i("[socket] connection #%u closed", client->socket());
        });
#endif

    websocketHandler.onFrame(
        [](PsychicWebSocketRequest *request, httpd_ws_frame *frame)
        {
            log_i("received websocket frame: %s", reinterpret_cast<char *>(frame->payload));

            String wsResponse = "recieved: \n";
            wsResponse += reinterpret_cast<char *>(frame->payload);

            return request->reply(wsResponse.c_str());
        });
}

static std::optional<uint8_t> validateChannel(PsychicRequest *request)
{
    constexpr char *CHANNEL = "channel";

    if (!request->hasParam(CHANNEL))
    {
        request->reply(400, TEXT_PLAIN, "No channel parameter provided");
        return std::nullopt;
    }

    const String &channelStr = request->getParam(CHANNEL)->value();

    if (channelStr.length() != 1 || channelStr[0] < '0' || channelStr[0] > '4')
    {
        request->reply(400, TEXT_PLAIN, "Invalid channel parameter (must be a number 0-4)");
        return std::nullopt;
    }

    return channelStr[0] - '0';
}

time_t time_diff(struct tm *start, struct tm *end)
{
    return mktime(end) - mktime(start);
}

static bool handleFileUpload(const String &file, const String &filePath, String &result)
{
    if (!SD.begin(SDCARD_SS))
    {
        result = "SD card initialization failed";
        return false;
    }

    File destFile = SD.open(filePath, FILE_WRITE);
    if (!destFile)
    {
        SD.end();
        result = "Failed to open file for writing";
        return false;
    }

    const size_t fileSize = file.length();

    if (destFile.write(reinterpret_cast<const uint8_t *>(file.c_str()), fileSize) != fileSize)
    {
        destFile.close();
        SD.end();
        result = "File write error";
        return false;
    }

    destFile.close();
    SD.end();
    result = "File saved successfully!";
    return true;
}

static void setupWebserverHandlers(PsychicHttpServer &server, tm *timeinfo)
{
    server.on(
        "/", HTTP_GET, [](PsychicRequest *request)
        {
            if (samePageIsCached(request, contentCreationTime, etagValue))
                return request->reply(304);

            extern const uint8_t index_start[] asm("_binary_src_webui_index_html_gz_start");
            extern const uint8_t index_end[] asm("_binary_src_webui_index_html_gz_end");

            PsychicResponse response = PsychicResponse(request);
            addStaticContentHeaders(response, contentCreationTime, etagValue);
            response.addHeader(CONTENT_ENCODING, GZIP);
            response.setContentType(TEXT_HTML);
            const size_t size = index_end - index_start;
            response.setContent(index_start, size);
            return response.send(); }

    );

    server.on(
        "/editor", HTTP_GET, [](PsychicRequest *request)
        {
            if (samePageIsCached(request, contentCreationTime, etagValue))
                return request->reply(304);

            extern const uint8_t editor_start[] asm("_binary_src_webui_editor_html_gz_start");
            extern const uint8_t editor_end[] asm("_binary_src_webui_editor_html_gz_end");   

            PsychicResponse response = PsychicResponse(request);
            addStaticContentHeaders(response, contentCreationTime, etagValue);
            response.addHeader(CONTENT_ENCODING, GZIP);
            response.setContentType(TEXT_HTML);
            const size_t size = editor_end - editor_start;
            response.setContent(editor_start, size);
            return response.send(); }

    );

    server.on(
        "/fileupload", HTTP_GET, [](PsychicRequest *request)
        {
            if (samePageIsCached(request, contentCreationTime, etagValue))
                return request->reply(304);

            extern const uint8_t upload_start[] asm("_binary_src_webui_fileupload_html_gz_start");
            extern const uint8_t upload_end[] asm("_binary_src_webui_fileupload_html_gz_end");   

            PsychicResponse response = PsychicResponse(request);
            addStaticContentHeaders(response, contentCreationTime, etagValue);
            response.addHeader(CONTENT_ENCODING, GZIP);
            response.setContentType(TEXT_HTML);
            const size_t size = upload_end - upload_start;
            response.setContent(upload_start, size);
            return response.send(); }

    );

    server.on(
        "/moonsetup", HTTP_GET, [](PsychicRequest *request)
        {
            if (samePageIsCached(request, contentCreationTime, etagValue))
                return request->reply(304);

            extern const uint8_t moon_start[] asm("_binary_src_webui_moonsetup_html_gz_start");
            extern const uint8_t moon_end[] asm("_binary_src_webui_moonsetup_html_gz_end");

            PsychicResponse response = PsychicResponse(request);
            addStaticContentHeaders(response, contentCreationTime, etagValue);
            response.addHeader(CONTENT_ENCODING, GZIP);
            response.setContentType(TEXT_HTML);
            const size_t size = moon_end - moon_start;
            response.setContent(moon_start, size);
            return response.send(); }

    );

    server.on(
        "/api/timers", HTTP_GET, [](PsychicRequest *request)
        {
            auto validChannel = validateChannel(request);
            if (!validChannel) 
                return ESP_OK;

            uint8_t channelIndex = *validChannel;

            String content;
            content.reserve(256);

            {
                std::lock_guard<std::mutex> lock(channelMutex);
                for (auto &timer : channel[channelIndex])
                    content += String(timer.time) + "," + String(timer.percentage) + "\n";
            }

            PsychicStreamResponse response(request, TEXT_PLAIN);

            response.addHeader("Cache-Control", "no-store, no-cache, must-revalidate, proxy-revalidate");
            response.addHeader("Pragma", "no-cache");
            response.addHeader("Expires", "0");

            response.setContent(content.c_str());
            return response.send(); }

    );

    server.on(
        "/api/timers", HTTP_POST, [](PsychicRequest *request)
        {
            auto validChannel = validateChannel(request);
            if (!validChannel)
                return ESP_OK;

            const uint8_t channelIndex = *validChannel;

            String csvData = request->body();

            std::vector<lightTimer_t> newTimers;
            int startIdx = 0;
            int endIdx = csvData.indexOf('\n');

            log_d("Parsing timers for channel %i", channelIndex);

            while (endIdx != -1)
            {
                String line = csvData.substring(startIdx, endIdx);

                if (line.length() == 0)
                    continue;

                int commaIdx = line.indexOf(',');

                if (commaIdx != -1)
                {
                    int time = strtol(line.substring(0, commaIdx).c_str(), NULL, 10);
                    int percentage = strtol(line.substring(commaIdx + 1).c_str(), NULL, 10);

                    if (time > 86400 || percentage > 100)
                    {
                        log_e("Timer data value overflow");
                        return request->reply(400, TEXT_PLAIN, "Overflow in timer data");
                    }

                    newTimers.push_back({time, percentage});

                    log_v("Staging% 6i,% 4i for channel %i", time, percentage, channelIndex);
                }

                startIdx = endIdx + 1;
                endIdx = csvData.indexOf('\n', startIdx);
            }

            if (newTimers.size() < 2 ||
                newTimers.front().time != 0 || newTimers.back().time != 86400 ||
                newTimers.front().percentage != newTimers.back().percentage)
            {
                log_e("Staged timerdata failed sanity check");
                return request->reply(400, TEXT_PLAIN, "Data sanity check failed");
            }

            {
                std::lock_guard<std::mutex> lock(channelMutex);
                channel[channelIndex].clear();
                for (auto &timer : newTimers)
                    channel[channelIndex].push_back(timer);
            }

            log_i("Cleared and added %i new timers to channel %i", channel[channelIndex].size(), channelIndex);

            String result;
            result.reserve(128);

            const bool success = saveDefaultTimers(result);

            return request->reply(success ? 200 : 500, TEXT_PLAIN, result.c_str()); }

        )
    ->setAuthentication(WEBIF_USER, WEBIF_PASSWORD);

    server.on(
        "/api/moonlevels", HTTP_GET, [](PsychicRequest *request)
        {
            String responseStr;
            responseStr.reserve(NUMBER_OF_CHANNELS * 8);

            {
                std::lock_guard<std::mutex> lock(channelMutex);
                for (int i = 0; i < NUMBER_OF_CHANNELS; i++)
                {
                    responseStr += String(fullMoonLevel[i]);
                    if (i < NUMBER_OF_CHANNELS - 1)
                        responseStr += ",";
                }
            }

            return request->reply(200, TEXT_PLAIN, responseStr.c_str()); }

    );

    server.on(
              "/api/moonlevels", HTTP_POST, [](PsychicRequest *request)
              {
                  String body = request->body();
                  float newLevels[NUMBER_OF_CHANNELS];

                  int start = 0, count = 0;
                  while (count < NUMBER_OF_CHANNELS)
                  {
                      int comma = body.indexOf(',', start);
                      String valueStr = (comma == -1) ? body.substring(start) : body.substring(start, comma);
                      start = comma + 1;

                      float value = valueStr.toFloat();
                      if (value < 0.0f || value > 1.0f)
                          return request->reply(400, TEXT_PLAIN, "Invalid values");

                      newLevels[count++] = value;
                      if (comma == -1)
                          break;
                  }

                  if (count != NUMBER_OF_CHANNELS)
                      return request->reply(400, TEXT_PLAIN, "Incorrect number of values");

                  {
                      std::lock_guard<std::mutex> lock(channelMutex);
                      for (int i = 0; i < NUMBER_OF_CHANNELS; i++)
                          fullMoonLevel[i] = newLevels[i];
                  }

                  String result;
                  result.reserve(128);

                  const bool success = saveMoonSettings(result);

                  return request->reply(success ? 200 : 500, TEXT_PLAIN, result.c_str()); }

              )
        ->setAuthentication(WEBIF_USER, WEBIF_PASSWORD);

    server.on(
              "/api/upload", HTTP_POST, [](PsychicRequest *request)
              {
                  constexpr char *PARAMETER_FILE_NAME = "filename";

                  if (!request->hasParam(PARAMETER_FILE_NAME) || request->getParam(PARAMETER_FILE_NAME)->value().isEmpty())
                      return request->reply(400, TEXT_PLAIN, "No filename provided");

                  String fileName = request->getParam(PARAMETER_FILE_NAME)->value();
                  const String filePath = "/" + fileName;

                  String file = request->body();

                  if (file.length() == 0)
                      return request->reply(400, TEXT_PLAIN, "File is empty");

                  String result;
                  result.reserve(128);
                  
                  bool success;
                  {
                      ScopedMutex scopedMutex(spiMutex);

                      if (!scopedMutex.acquired())
                      {
                          log_w("Mutex timeout");
                          return request->reply(500, TEXT_PLAIN, "Server busy, try again later");
                      }
                      success = handleFileUpload(file, filePath, result);
                  }

                  if (!success)
                      return request->reply(500, TEXT_PLAIN, result.c_str());

                  if (!strcmp(DEFAULT_TIMERFILE, filePath.c_str()))
                      success = loadDefaultTimers(result);
                  else if (!strcmp(MOON_SETTINGS_FILE, filePath.c_str()))
                      success = loadMoonSettings(result);

                  return request->reply(success ? 200 : 500, TEXT_PLAIN, result.c_str()); }

              )
        ->setAuthentication(WEBIF_USER, WEBIF_PASSWORD);

    server.on(
        "/api/uptime", HTTP_GET, [&timeinfo](PsychicRequest *request)
        {
            time_t now;
            time(&now);

            struct tm currentTime;
            gmtime_r(&now, &currentTime);

            time_t uptimeSeconds = time_diff(timeinfo, &currentTime);

            int years = uptimeSeconds / (60 * 60 * 24 * 365);
            uptimeSeconds %= (60 * 60 * 24 * 365);
            int days = uptimeSeconds / (60 * 60 * 24);
            uptimeSeconds %= (60 * 60 * 24);
            int hours = uptimeSeconds / (60 * 60);
            uptimeSeconds %= (60 * 60);
            int minutes = uptimeSeconds / 60;
            int seconds = uptimeSeconds % 60;

            String uptimeString = "";
            if (years > 0)
                uptimeString += String(years) + " years, ";
            if (days > 0)
                uptimeString += String(days) + " days, ";
            if (hours > 0)
                uptimeString += String(hours) + " hours, ";
            if (minutes > 0)
                uptimeString += String(minutes) + " minutes and ";
            uptimeString += String(seconds) + " seconds";

            return request->reply(200, TEXT_PLAIN, uptimeString.c_str()); }

    );

#if defined(CORE_DEBUG_LEVEL) && (CORE_DEBUG_LEVEL >= 4)
    server.on(
        "/api/taskstats", HTTP_GET, [](PsychicRequest *request)
        {
            uint32_t totalRunTime;
            UBaseType_t taskCount = uxTaskGetNumberOfTasks();

            TaskStatus_t *pxTaskStatusArray = (TaskStatus_t *)heap_caps_malloc(taskCount * sizeof(TaskStatus_t), MALLOC_CAP_INTERNAL);
            if (!pxTaskStatusArray) {
                return request->reply(500, TEXT_PLAIN, "Memory allocation failed");
            }

            UBaseType_t retrievedTasks = uxTaskGetSystemState(pxTaskStatusArray, taskCount, &totalRunTime);
            if (totalRunTime == 0 || retrievedTasks == 0) {
                heap_caps_free(pxTaskStatusArray);
                return request->reply(500, TEXT_PLAIN, "Failed to get task stats");
            }

            String csvResponse = "Name,State,Priority,Stack,Runtime,CPU%\n";

            for (UBaseType_t i = 0; i < retrievedTasks; i++) {
                const char *taskName = pxTaskStatusArray[i].pcTaskName;

                float cpuPercent = ((float)pxTaskStatusArray[i].ulRunTimeCounter / (float)totalRunTime) * 100.0f;

                csvResponse += String(taskName) + "," +
                            String(pxTaskStatusArray[i].eCurrentState) + "," +
                            String(pxTaskStatusArray[i].uxCurrentPriority) + "," +
                            String(pxTaskStatusArray[i].usStackHighWaterMark) + "," +
                            String(pxTaskStatusArray[i].ulRunTimeCounter) + "," +
                            String(cpuPercent, 2) + "\n";
            }

            heap_caps_free(pxTaskStatusArray);

            return request->reply(200, TEXT_PLAIN, csvResponse.c_str()); }

    );

    server.on(
        "/stats", HTTP_GET, [](PsychicRequest *request)
        {
            if (samePageIsCached(request, contentCreationTime, etagValue))
                return request->reply(304);

            extern const uint8_t stats_start[] asm("_binary_src_webui_stats_html_gz_start");
            extern const uint8_t stats_end[] asm("_binary_src_webui_stats_html_gz_end");   

            PsychicResponse response = PsychicResponse(request);
            addStaticContentHeaders(response, contentCreationTime, etagValue);
            response.addHeader(CONTENT_ENCODING, GZIP);
            response.setContentType(TEXT_HTML);
            const size_t size =(stats_end - stats_start);
            response.setContent(stats_start, size);
            return response.send(); }

    );
#endif

    server.onNotFound(
        [](PsychicRequest *request)
        {
            log_e("404 - Not found: 'http://%s%s'", request->host().c_str(), request->url().c_str());
            return request->reply(404, TEXT_HTML, "<h1>404 - Not found</h1>"); }

    );

#define SHOW_HTTP_CONNECTIONS 0
#if SHOW_HTTP_CONNECTIONS
    server.onOpen(
        [](PsychicClient *client)
        {
            log_i("[http] connection #%u connected from %s", client->socket(), client->remoteIP().toString().c_str());
        });

    server.onClose(
        [](PsychicClient *client)
        {
            log_i("[http] connection #%u closed", client->socket());
        });
#endif
}

void httpTask(void *parameter)
{
    if (!websocketQueue)
    {
        log_e("fatal error. No websocketqueue. system halted.");
        while (1)
            delay(100);
    }

    const time_t rawTime = time(NULL);
    struct tm *timeinfo = gmtime(&rawTime);
    strftime(contentCreationTime, sizeof(contentCreationTime), "%a, %d %b %Y %X GMT", timeinfo);

    generateETag(contentCreationTime);

    static PsychicHttpServer server;
    static PsychicWebSocketHandler websocketHandler;

    server.config.max_uri_handlers = 15;
    server.config.max_open_sockets = 8;

    server.listen(80);

    setupWebsocketHandler(websocketHandler);
    server.on("/websocket", HTTP_GET, &websocketHandler);

    setupWebserverHandlers(server, timeinfo);
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    log_i("HTTP server started at %s", WiFi.localIP().toString());

    while (1)
    {
        static websocketMessage msg;
        if (xQueueReceive(websocketQueue, &msg, portMAX_DELAY))
        {
            switch (msg.type)
            {
            case LIGHT_UPDATE:
                log_v("light update msg received: %s", msg.str);
                if (websocketHandler.count())
                    websocketHandler.sendAll(msg.str);
                break;

            case TEMPERATURE_UPDATE:
                log_v("temperature update msg received: %s", msg.str);
                if (websocketHandler.count())
                    websocketHandler.sendAll(msg.str);
                break;

            default:
                break;
            }
        }
    }
}
