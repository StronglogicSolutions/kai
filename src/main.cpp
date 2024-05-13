#include <iostream>
#include <string>
#include <string_view>
#include <curl/curl.h>
#include <logger.hpp>
#include <simdjson.h>
#include <cstring>
#include <kutils.hpp>

//--------------------------------------------------------

using namespace kiq::log;

//--------------------------------------------------------
static const char* url = "http://127.0.0.1:8080/completion"; // LLAMA AI SERVER

//--------------------------------------------------------
//--------------HELPERS-----------------------------------
//--------------------------------------------------------
size_t curl_callback(void *contents, size_t size, size_t nmemb, std::string *buffer)
{
  size_t totalSize = size * nmemb;
  buffer->append((char*)contents, totalSize);
  return totalSize;
}
//--------------------------------------------------------
std::string
handle_response(const std::string& response)
{
  const size_t                       i_sz = response.size();
  const size_t                       size = i_sz + simdjson::SIMDJSON_PADDING;
  const auto                         data = simdjson::padded_string{response.data(), response.size()};

        std::string                  ret;

        simdjson::ondemand::parser   parser;
        simdjson::ondemand::document doc = parser.iterate(data);

  if (ret = std::string(doc["content"].get_string().value()); ret.empty())
    klog().e("Failed to parse content");
  else
    klog().i("Content: {}", ret);

  return ret;
}
//--------------------------------------------------------
void post(std::string body, std::string url)
{
  if (CURL *curl = curl_easy_init(); curl)
  {
    std::string        response;
    struct curl_slist* headers  = nullptr;
                       headers  = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());   // URL
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body.c_str());  // Post data
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);       // Headers
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback); // Callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);

    if (CURLcode res = curl_easy_perform(curl); res != CURLE_OK)
      klog().e("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    else
    {
      klog().d("POST request successful. Response:\n{}", response);
      handle_response(response);
    }

    curl_easy_cleanup  (curl);
    curl_slist_free_all(headers);
  }
  else
  {
    klog().e("Failed to initialize libcurl");
  }
}
//--------------------------------------------------------
std::string
make_query(const std::string& s)
{
  return R"({"prompt": "')" + s + R"('"})";
}
//--------------------------------------------------------
//-----------MAIN-----------------------------------------
//--------------------------------------------------------
int main(int argc, char** argv)
{
  auto input = kutils::kargs{argc, argv}.get("query");
  if (input.empty())
    input = "How are you?";

  klogger::init("kai", "debug");

  post(make_query(input), url);

  return 0;
}
