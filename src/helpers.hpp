#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <curl/curl.h>
#include <logger.hpp>
#include <simdjson.h>
#include <kutils.hpp>
#include <wait.h>

//--------------------------------------------------------
using namespace kiq::log;
using curl_t = CURL;
//--------------------------------------------------------
struct search_data
{
  std::string title;
  std::string snippet;
  std::string link;
};
//--------------------------------------------------------
using websearch_data_t = std::vector<search_data>;
//--------------------------------------------------------
static const std::string g_app_name = "KAI";
//--------------------------------------------------------
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
static std::string
make_query(const std::string& s)
{
  return R"({"prompt": "')" + s + R"('"})";
}
//--------------------------------------------------------
static std::string
get_sanitized_string(std::string_view s)
{
  std::string ret;
  for (char c : s)
  {
    if (c == '\n')
      ret += "\\n";
    else
      ret += c;
  }
  return ret;
}
//--------------------------------------------------------
static size_t
callback(void *contents, size_t size, size_t nmemb, std::string *buffer)
{
  size_t total_size = size * nmemb;
  buffer->append((char*)contents, total_size);
  return total_size;
}
//--------------------------------------------------------
static std::string
decode(const std::string& response)
{
  if (response.empty())
    return "";

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
static websearch_data_t
get_google_result(const std::string& response)
{
    if (response.empty())
    return {};

  const size_t                       i_sz = response.size();
  const size_t                       size = i_sz + simdjson::SIMDJSON_PADDING;
  const auto                         data = simdjson::padded_string{response.data(), i_sz};

  websearch_data_t                          ret;

        simdjson::ondemand::parser   parser;
        simdjson::ondemand::document doc = parser.iterate(data);

  for (auto item : doc["items"].get_array())
  {
    auto obj = item.get_object().value();

    ret.push_back({ std::string{ obj["title"]  .get_string().value()},
                    std::string{ obj["snippet"].get_string().value()},
                    std::string{ obj["link"]   .get_string().value()} });
  }

  return ret;
}
//--------------------------------------------------------
static ai_payload_t
get_payload(const std::string& response)
{
  klog().t("Getting payload from:\n{}", response);
  auto s = response;
  std::replace_if(s.begin(), s.end(), [](auto c) { return c == '\"'; } , '"');

  const size_t                       i_sz = response.size();
  const size_t                       size = i_sz + simdjson::SIMDJSON_PADDING;
  const auto                         data = simdjson::padded_string{s.data(), s.size()};

        simdjson::ondemand::parser   parser;
        simdjson::ondemand::document doc = parser.iterate(data);

        std::string                  value;
        ai_payload_t                 ret;

  auto i = 0;
  for (std::string_view arg : doc["args"].get_array())
  {
    klog().t("From args array: {}", arg.data());
    ret.push(arg);
  }

  return ret;
}
//--------------------------------------------------------
static std::string
post(const std::string& query, const std::string& url)
{
  klog().d("Post request to {} with query {}", url, query);
  std::string response;
  const auto  data = make_query(get_sanitized_string(query));
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
//--------------------------------------------------------
static std::string
get(const std::string& url)
{
  klog().d("GET request to {}", url);
  std::string response;

  if (curl_t* curl = curl_easy_init(); curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, url.data());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    if (CURLcode res = curl_easy_perform(curl); res != CURLE_OK)
      klog().e("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    else
    {
      klog().d("Received response from server");
      klog().t("{}", response);
    }

    curl_easy_cleanup(curl);
  }
  else
    klog().e("Failed to initialize libcurl");

  return response;
}
//--------------------------------------------------------
static websearch_data_t
search_google(std::string_view query)
{
  static const auto path = kutils::GetCWD(g_app_name.size()) + "/../";
  static const auto base_url = "https://content-customsearch.googleapis.com/customsearch/v1";
  //-------------
  auto urlencode = [](const std::string& input)
  {
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase << std::setfill('0');

    for (unsigned char c : input)
    {
      if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        encoded << c; // Unreserved characters
      else
        encoded << '%' << std::setw(2) << static_cast<int>(c);
    }
    return encoded.str();
  };
  //-------------
  const auto key      = kutils::ReadFile(path + ".searchkey");
  const auto cx       = kutils::ReadFile(path + ".cx");
  const auto query_s  = "?q=" + urlencode(query.data()) + "&cx=" + cx + "&key=" + key;

  return get_google_result(get(base_url + query_s));
}

static std::string
process_llm_comment(std::string_view comment)
{
  const std::filesystem::path output_file = "/tmp/llmcodechat_output.txt";

  const auto command = "ollama run qwen2.5-coder:1.5b \"" + std::string{comment} + "\" > " + output_file.string();

  pid_t pid = fork();
  if (pid < 0)
    throw std::runtime_error("Failed to fork process");

  if (pid == 0) // Child process
  {
    execlp("zsh", "zsh", "-c", command.c_str(), nullptr);

    std::exit(1); // On Fail
  }

  // Parent process
  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
  {
    std::ifstream file(output_file);
    if (!file)
      throw std::runtime_error("Failed to open output file: " + output_file.string());

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    kutils::SaveToFile(content, std::string{kutils::get_unix_tstring() + ".txt"});

    klog().t("Received content from LLM: {}", content);

    // Find last occurrence of comment in output
    if (const auto last_pos = content.rfind(comment); last_pos != std::string::npos)
    {
      auto result = content.substr(last_pos + comment.length());

      size_t start = result.find_first_not_of(" \t\r\n"); // Trim
      size_t end = result.find_last_not_of(" \t\r\n");

      if (start != std::string::npos && end != std::string::npos)
        return result.substr(start, end - start + 1);
    }

    if (content.empty())
      throw std::runtime_error("No output after last occurrence of comment");

    return content;
  }

  throw std::runtime_error("llmcodechat exited with code " +
                          std::to_string(WEXITSTATUS(status)));
}
