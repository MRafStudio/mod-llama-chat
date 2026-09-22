#ifndef MOD_LLAMA_WOW_HTTPCLIENT_H
#define MOD_LLAMA_WOW_HTTPCLIENT_H

#include <string>

// Full result of an HTTP call, so callers can distinguish "model refused" from
// "server unreachable" -- the old client returned an empty string for both,
// which is why an unsupported think mode looked identical to a dead LlamaWow.
struct LlamaWowHttpResult
{
    int         status = 0;      // HTTP status, 0 when the request never completed
    std::string body;
    std::string error;           // transport-level error text, empty on success

    bool ok() const { return status == 200; }
};

class LlamaWowHttpClient
{
public:
    LlamaWowHttpClient();
    ~LlamaWowHttpClient();

    // Backwards-compatible form: body on 200, empty string otherwise.
    std::string Post(const std::string& url, const std::string& jsonData);

    // Preferred form. Connections are pooled per worker thread, so repeated
    // calls reuse a keep-alive socket without serialising concurrent workers
    // behind a single shared client.
    // timeoutOverride > 0 uses that instead of the configured timeout; the
    // capability probe wants to fail fast rather than hang startup diagnostics.
    LlamaWowHttpResult PostEx(const std::string& url, const std::string& jsonData,
                            int timeoutOverride = 0);

    // [MRafStudio fork] То же, но с необязательным Bearer-токеном.
    // Нужно OpenAI-совместимому режиму: серверы запускают с --api-key,
    // и без заголовка Authorization они отвечают 401.
    LlamaWowHttpResult PostExWithToken(const std::string& url, const std::string& jsonData,
                                     const std::string& bearerToken, int timeoutOverride = 0);
    LlamaWowHttpResult GetEx(const std::string& url, int timeoutOverride = 0);

    void SetTimeout(int seconds);
    bool IsAvailable() const;

private:
    int  m_timeout;
    bool m_available;
};

// Strip the API path off a configured endpoint so sibling endpoints can be
// derived: "http://host:11434/api/generate" -> "http://host:11434"
std::string LlamaWowDeriveBaseUrl(const std::string& url);

#endif // MOD_LLAMA_WOW_HTTPCLIENT_H
