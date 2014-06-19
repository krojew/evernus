#pragma once

namespace Evernus
{
    namespace NetworkSettings
    {
        const auto defaultAPIProvider = "https://api.eveonline.com";

        const auto useProxyKey = "network/useProxy";
        const auto proxyTypeKey = "network/proxy/type";
        const auto proxyHostKey = "network/proxy/host";
        const auto proxyPortKey = "network/proxy/post";
        const auto proxyUserKey = "network/proxy/user";
        const auto proxyPasswordKey = "network/proxy/password";

        const auto useCustomProvider = "network/useCustomProvider";
        const auto providerHost = "network/provider/host";
    }
}
