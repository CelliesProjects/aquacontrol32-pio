#include "httpTask.hpp"

extern const uint8_t editor_start[] asm("_binary_src_webui_editor_html_start");
extern const uint8_t editor_end[] asm("_binary_src_webui_editor_html_end");

extern std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
extern std::mutex channelMutex;

const char *TEXT_HTML = "text/html";
const char *TEXT_PLAIN = "text/plain";

void httpTask(void *parameter)
{
    server.listen(80);

    server.on(
        "/", HTTP_GET, [](PsychicRequest *request)
        {
            PsychicResponse response = PsychicResponse(request);
            response.setContentType(TEXT_HTML);
            size_t size = editor_end - editor_start;
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
                {
                    if (timer.time == 86400)
                        continue;

                    content += String(timer.time) + "," + String(timer.percentage) + "\n";
                }
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

            log_i("Wrote %i new timers to channel %i",channel[choice].size(), choice);

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