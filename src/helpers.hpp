#include <string>
#include <string_view>
#include <curl/curl.h>
#include <logger.hpp>
#include <simdjson.h>
//--------------------------------------------------------
using namespace kiq::log;
//--------------------------------------------------------
namespace
{
  using handler_t = std::function<void(std::string, std::string)>;
  handler_t g_handler = nullptr;
}
//--------------------------------------------------------
void set_handler(handler_t handler)
{
  g_handler = handler;
}
//--------------------------------------------------------
static const char* url = "http://127.0.0.1:8080/completion"; // LLAMA AI SERVER
//--------------------------------------------------------
//--------------HELPERS-----------------------------------
//--------------------------------------------------------
std::string
make_query(const std::string& s)
{
  return R"({"prompt": "')" + s + R"('"})";
}
//--------------------------------------------------------
size_t
curl_callback(void *contents, size_t size, size_t nmemb, std::string *buffer)
{
  size_t total_size = size * nmemb;
  buffer->append((char*)contents, total_size);
  return total_size;
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
// Using curl
void
post(std::string_view query, std::string_view url)
{
  const auto body = make_query(query.data());

  if (CURL *curl = curl_easy_init(); curl)
  {
           std::string response;
    struct curl_slist* headers  = nullptr;
                       headers  = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL,           url.data ());   // URL
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body.data());   // Post data
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);       // Headers
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback); // Callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);

    if (CURLcode res = curl_easy_perform(curl); res != CURLE_OK)
      klog().e("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    else
    {
      klog().d("POST request successful. Response:\n{}", response);
      const auto data = handle_response(response);
      g_handler(query.data(), data);
    }

    curl_easy_cleanup  (curl);
    curl_slist_free_all(headers);
  }
  else
    klog().e("Failed to initialize libcurl");
}