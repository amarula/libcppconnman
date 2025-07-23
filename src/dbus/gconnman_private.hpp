
namespace Amarula::DBus::G::Connman {

constexpr auto SERVICE = "net.connman";
constexpr auto OBJECT_PATH = "/net/connman";
constexpr auto MANAGER_PATH = "/";

constexpr auto NAME_STR = "Name";
constexpr auto TYPE_STR = "Type";
constexpr auto ERROR_STR = "Error";
constexpr auto AUTO_STR = "auto";
constexpr auto MANUAL_STR = "manual";
constexpr auto TIMESERVERS_STR = "Timeservers";

// Clock interface
constexpr auto CLOCK_INTERFACE = "net.connman.Clock";
constexpr auto TIME_STR = "Time";
constexpr auto TIMEUPDATES_STR = "TimeUpdates";
constexpr auto TIMEZONE_STR = "Timezone";
constexpr auto TIMEZONEUPDATES_STR = "TimezoneUpdates";
constexpr auto TIMESERVERSYNCED_STR = "TimeserverSynced";

// Technology interface
constexpr auto TECHNOLOGY_INTERFACE = "net.connman.Technology";
constexpr auto POWERED_STR = "Powered";
constexpr auto CONNECTED_STR = "Connected";
constexpr auto TETHERING_STR = "Tethering";
constexpr auto TETHERINGIDENTIFIER_STR = "TetheringIdentifier";
constexpr auto TETHERINGPASSPHRASE_STR = "TetheringPassphrase";
constexpr auto TETHERINGFREQ_STR = "TetheringFreq";
constexpr auto SCAN_STR = "Scan";

// Service interface
constexpr auto SERVICE_INTERFACE = "net.connman.Service";
constexpr auto AUTOCONNECT_STR = "AutoConnect";
constexpr auto FAVORITE_STR = "Favorite";
constexpr auto IMMUTABLE_STR = "Immutable";
constexpr auto MDNS_STR = "mDNS";
constexpr auto STRENGTH_STR = "Strength";
constexpr auto IPV4_STR = "IPv4";
constexpr auto IPV6_STR = "IPv6";
constexpr auto NAMESERVERS_STR = "Nameservers";
constexpr auto DOMAINS_STR = "Domains";
constexpr auto METHOD_STR = "Method";
constexpr auto ADDRESS_STR = "Address";
constexpr auto NETMASK_STR = "Netmask";
constexpr auto GATEWAY_STR = "Gateway";
constexpr auto PRIVACY_STR = "Privacy";
constexpr auto PREFIXLENGTH_STR = "PrefixLength";
constexpr auto ETHERNET_STR = "Ethernet";
constexpr auto PROVIDER_STR = "Provider";
constexpr auto HOST_STR = "Host";
constexpr auto DOMAIN_STR = "Domain";
constexpr auto PROXY_STR = "Proxy";
constexpr auto URL_STR = "Url";
constexpr auto SERVERS_STR = "Servers";
constexpr auto EXCLUDES_STR = "Excludes";
constexpr auto SECURITY_STR = "Security";
constexpr auto CONNECT_STR = "Connect";
constexpr auto DISCONNECT_STR = "Disconnect";
constexpr auto REMOVE_STR = "Remove";
constexpr auto INTERFACE_STR = "Interface";
constexpr auto MTU_STR = "MTU";

// Manager interface
constexpr auto MANAGER_INTERFACE = "net.connman.Manager";
constexpr auto OFFLINEMODE_STR = "OfflineMode";
constexpr auto GETTECHNOLOGIES_STR = "GetTechnologies";
constexpr auto GETSERVICES_STR = "GetServices";
constexpr auto STATE_STR = "State";
constexpr auto REGISTERAGENT_STR = "RegisterAgent";
constexpr auto UNREGISTERAGENT_STR = "UnregisterAgent";

}  // namespace Amarula::DBus::G::Connman
