#include "httpTask.hpp"

extern const uint8_t editor_start[] asm("_binary_src_webui_editor_html_start");
extern const uint8_t editor_end[] asm("_binary_src_webui_editor_html_end");

void httpTask(void *parameter)
{
    server.listen(80);

    server.on(
        "/", HTTP_GET, [](PsychicRequest *request)
        {
        PsychicResponse response = PsychicResponse(request);
        response.setContentType("text/html");
        size_t size = editor_end - editor_start;        
        response.setContent(editor_start, size);        
        return response.send(); });

    server.onNotFound(
        [](PsychicRequest *request)
        {
            log_e("404 - Not found: 'http://%s%s'", request->host().c_str(), request->url().c_str());
            return request->reply(404, "text/html", "<h1>FOUR OH FOUR NOT FOUND</h1>"); });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    // Log that the server is running
    log_i("HTTP server started at %s", WiFi.localIP().toString());
    // messageOnLcd(const char *str);

    // Main task loop
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}