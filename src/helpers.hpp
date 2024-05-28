#pragma once

#include <string>
#include <string_view>
#include <curl/curl.h>
#include <logger.hpp>
#include <simdjson.h>
#include <kutils.hpp>

//--------------------------------------------------------
using namespace kiq::log;
using curl_t = CURL;

struct ai_payload_t
{
  ai_payload_t() {}

  void
  push(std::string_view v)
  {
    std::string&& s = v.data();
    klog().d("Pushing back {}", s);
    values.emplace_back(std::move(s));
  }

  std::string type() const
  {
    return values.at(0);
  }

  std::string data() const
  {
    return values.at(1);
  }

  std::vector<std::string> values;
};
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
callback(void *contents, size_t size, size_t nmemb, std::string *buffer)
{
  size_t total_size = size * nmemb;
  buffer->append((char*)contents, total_size);
  return total_size;
}
//--------------------------------------------------------
std::string
decode(const std::string& response)
{
  const size_t                       i_sz = response.size();
  const size_t                       size = i_sz + simdjson::SIMDJSON_PADDING;
  const auto                         data = simdjson::padded_string{response.data(), i_sz};

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
ai_payload_t
get_payload(const std::string& response)
{
  auto s = response;
  std::replace_if(s.begin(), s.end(), [](auto c) { return c == '\"'; } , '"');

  const size_t                       i_sz = response.size();
  const size_t                       size = i_sz + simdjson::SIMDJSON_PADDING;
  const auto                         data = simdjson::padded_string{s.data(), s.size()};

        simdjson::ondemand::parser   parser;
        simdjson::ondemand::document doc = parser.iterate(data);

        std::string                  value;
        ai_payload_t                 ret;

  for (auto arg : doc["args"].get_array())
  {
    value = arg.get_string().value();
    ret.push(value);
  }

  return ret;
}
//--------------------------------------------------------
std::string
post(std::string_view query, std::string_view url)
{
  std::string response;
  const auto  data = make_query(query.data());
  if (curl_t *curl = curl_easy_init(); curl)
  {
    struct curl_slist* headers  = nullptr;
                       headers  = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL,           url.data ());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    data.data());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);

    if (CURLcode res = curl_easy_perform(curl); res != CURLE_OK)
      klog().e("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    else
    {
      klog().d("Received response from AI");
      klog().t("{}", response);
    }

    curl_easy_cleanup  (curl);
    curl_slist_free_all(headers);
  }
  else
    klog().e("Failed to initialize libcurl");

  return response;
}