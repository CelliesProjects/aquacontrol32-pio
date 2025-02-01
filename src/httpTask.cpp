#include "httpTask.hpp"

extern std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
extern std::mutex channelMutex;

constexpr char *TEXT_HTML = "text/html";
constexpr char *TEXT_PLAIN = "text/plain";

constexpr char *IF_MODIFIED_SINCE = "If-Modified-Since";
constexpr char *IF_NONE_MATCH = "If-None-Match";

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

// todo: this can be done at compile time
static void generateETag(const char *date)
{
    uint32_t hash = 0;
    for (const char *p = date; *p; ++p)
        hash = (hash * 31) + *p; // Simple hash function

    snprintf(etagValue, sizeof(etagValue), "\"%" PRIX32 "\"", hash);
}

void httpTask(void *parameter)
{
    time_t rawTime = time(NULL); // TODO: change this to compile time like in the feather player project
    const struct tm *timeinfo = gmtime(&rawTime);

    static char lastModified[30];
    strftime(lastModified, sizeof(lastModified), "%a, %d %b %Y %X GMT", timeinfo);

    generateETag(lastModified);

    server.listen(80);

    server.on(
        "/", HTTP_GET, [](PsychicRequest *request)
        {
            if (samePageIsCached(request, lastModified, etagValue))
                return request->reply(304);

            extern const uint8_t index_start[] asm("_binary_src_webui_index_html_start");
            extern const uint8_t index_end[] asm("_binary_src_webui_index_html_end");

            PsychicResponse response = PsychicResponse(request);
            addStaticContentHeaders(response, lastModified, etagValue);
            response.setContentType(TEXT_HTML);
            const size_t size = index_end - index_start;
            response.setContent(index_start, size);
            return response.send(); }

    );

    server.on(
        "/editor", HTTP_GET, [](PsychicRequest *request)
        {
            if (samePageIsCached(request, lastModified, etagValue))
                return request->reply(304);

            extern const uint8_t editor_start[] asm("_binary_src_webui_editor_html_start");
            extern const uint8_t editor_end[] asm("_binary_src_webui_editor_html_end");   

            PsychicResponse response = PsychicResponse(request);
            addStaticContentHeaders(response, lastModified, etagValue);
            response.setContentType(TEXT_HTML);
            const size_t size = editor_end - editor_start;
            response.setContent(editor_start, size);
            return response.send(); }

    );

    server.on(
        "/timers", HTTP_GET, [](PsychicRequest *request)
        {
            if (!request->hasParam("channel"))
                return request->reply(400, TEXT_PLAIN, "No channel parameter provided");

            const uint8_t choice = strtol(request->getParam("channel")->value().c_str(), NULL, 10);

            if (choice >= NUMBER_OF_CHANNELS)
                return request->reply(400, TEXT_PLAIN, "Valid channels are 0-4");

            String content;
            content.reserve(256);

            {
                std::lock_guard<std::mutex> lock(channelMutex);
                for (auto &timer : channel[choice])
                    content += String(timer.time) + "," + String(timer.percentage) + "\n";
            }

            return request->reply(200, TEXT_PLAIN, content.c_str()); }

    );

    server.on(
        "/upload", HTTP_POST, [](PsychicRequest *request)
        {
            if (!request->hasParam("channel"))
                return request->reply(400, TEXT_PLAIN, "No channel parameter provided");

            const uint8_t choice = strtol(request->getParam("channel")->value().c_str(), NULL, 10);

            if (choice >= NUMBER_OF_CHANNELS)
                return request->reply(400, TEXT_PLAIN, "Valid channels are 0-4");

            String csvData = request->body();

            std::vector<lightTimer_t> newTimers;
            int startIdx = 0;
            int endIdx = csvData.indexOf('\n');

            log_d("Parsing timers for channel %i", choice);

            while (endIdx != -1)
            {
                String line = csvData.substring(startIdx, endIdx);

                if (line.length() == 0) continue;

                int commaIdx = line.indexOf(',');

                if (commaIdx != -1)
                {
                    int time = strtol(line.substring(0, commaIdx).c_str(), NULL, 10);
                    int percentage = strtol(line.substring(commaIdx + 1).c_str(), NULL, 10);

                    if (time > 86400 || percentage > 100)
                    {
                        log_e("Timer data value overflow");
                        return request->reply(400, TEXT_PLAIN, "Invalid timer data");
                    }

                    newTimers.push_back({time, percentage});

                    log_v("Staging% 6i,% 4i for channel %i", time, percentage, choice);
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
                channel[choice].clear();
                for (auto &timer : newTimers)
                    channel[choice].push_back(timer);
            }

            log_i("Cleared and added %i new timers to channel %i",channel[choice].size(), choice);

            return request->reply(200, TEXT_PLAIN, "Timers updated successfully"); }

    );

    server.onNotFound(
        [](PsychicRequest *request)
        {
            log_e("404 - Not found: 'http://%s%s'", request->host().c_str(), request->url().c_str());
            return request->reply(404, TEXT_HTML, "<h1>FOUR OH FOUR NOT FOUND</h1>"); }

    );

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    log_i("HTTP server started at %s", WiFi.localIP().toString());

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
