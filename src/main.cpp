#include <iostream>
#include <string>
#include <curl/curl.h>
#include <logger.hpp>

using namespace kiq::log;

size_t curl_callback(void *contents, size_t size, size_t nmemb, std::string *buffer)
{
  size_t totalSize = size * nmemb;
  buffer->append((char*)contents, totalSize);
  return totalSize;
}

void post(std::string body, std::string url)
{
  CURL *curl;
  CURLcode res;
  struct curl_slist *headers = NULL;

  // Initialize libcurl
  curl = curl_easy_init();
  if (curl)
  {
    // Set the URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Set the POST data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());

    // Set content type header
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set callback function to receive response
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Perform the request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      klog().e("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    }
    else
    {
      klog().i("POST request successful. Response:\n{}", response);
    }

    // Clean up
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
  }
  else
  {
    klog().e("Failed to initialize libcurl");
  }
}

std::string
make_query(const std::string& s)
{
  return "{\"prompt\": \"'" + s + "'\"}";
}

int main()
{
  klogger::init("kai", "debug");

  std::string url = "http://127.0.0.1:8080/completion";
  std::string data = make_query("How are you?");

  post(data, url);

  return 0;
}
