/**
 * @file
 */

#pragma once

namespace cfg {

constexpr const char *ClientMouseRotationSpeed = "cl_cammouserotspeed";
constexpr const char *ClientMouseSpeed = "cl_cammousespeed";
constexpr const char *ClientEmail = "cl_email";
constexpr const char *ClientPassword = "cl_password";
// name of the player that is visible by other players
constexpr const char *ClientName = "cl_name";
constexpr const char *ClientVSync = "cl_vsync";
// the port where the server is listening on that the client wants to connect to
constexpr const char *ClientPort = "cl_port";
// the host where the server is running on that the client wants to connect to
constexpr const char *ClientHost = "cl_host";
constexpr const char *ClientFullscreen = "cl_fullscreen";
constexpr const char *ClientMultiSampleSamples = "cl_multisamplesamples";
constexpr const char *ClientMultiSampleBuffers = "cl_multisamplebuffers";
constexpr const char *ClientShadowMapSize = "cl_shadowmapsize";
constexpr const char *ClientOpenGLVersion = "cl_openglversion";
constexpr const char *ClientRenderUI = "cl_renderui";
constexpr const char *ClientWindowWidth = "cl_width";
constexpr const char *ClientWindowDisplay = "cl_display";
constexpr const char *ClientWindowHeight = "cl_height";
constexpr const char *ClientWindowHighDPI = "cl_highdpi";
constexpr const char *UIShowMetrics = "ui_showmetrics";
constexpr const char *UIFontSize = "ui_fontsize";
constexpr const char *UILastDirectory = "ui_lastdirectory";
// filedialog
constexpr const char *UIShowHidden = "ui_showhidden";
constexpr const char *UINotifyDismissMillis = "ui_notifydismiss";
constexpr const char *UIBookmarks = "ui_bookmarks";

constexpr const char *AudioSoundVolume = "snd_volume";
constexpr const char *AudioMusicVolume = "snd_musicvolume";

constexpr const char *ClientGamma = "cl_gamma";
constexpr const char *ClientShadowMap = "cl_shadowmap";
constexpr const char *ClientWater = "cl_water";
constexpr const char *ClientFog = "cl_fog";
constexpr const char *ClientCameraMaxTargetDistance = "cl_cameramaxtargetdistance";
constexpr const char *ClientCameraZoomSpeed = "cl_camzoomspeed";

constexpr const char *ClientDebugShadowMapCascade = "cl_debug_cascade";
constexpr const char *ClientDebugShadow = "cl_debug_shadow";

constexpr const char *RenderOutline = "r_renderoutline";

constexpr const char *ServerUserTimeout = "sv_usertimeout";
// the server side seed that is used to create the world
constexpr const char *ServerSeed = "sv_seed";
constexpr const char *ServerHost = "sv_host";
constexpr const char *ServerPort = "sv_port";
constexpr const char *ServerMaxClients = "sv_maxclients";
constexpr const char *ServerPostgresLib = "sv_postgreslib";
constexpr const char *ServerHttpPort = "sv_httpport";
// the download urls for the chunks
constexpr const char *ServerChunkBaseUrl = "sv_httpchunkurl";

constexpr const char *ConsoleCurses = "con_curses";

constexpr const char *CoreMaxFPS = "core_maxfps";
constexpr const char *CoreLogLevel = "core_loglevel";
constexpr const char *CoreSysLog = "core_syslog";
constexpr const char *CorePath = "core_path";

// The size of the chunk that is extracted with each step
constexpr const char *VoxelMeshSize = "voxel_meshsize";

constexpr const char *DatabaseName = "db_name";
constexpr const char *DatabaseHost = "db_host";
constexpr const char *DatabasePort = "db_port";
constexpr const char *DatabasePassword = "db_pw";
constexpr const char *DatabaseUser = "db_user";
constexpr const char *DatabaseMinConnections = "db_minconnections";
constexpr const char *DatabaseMaxConnections = "db_maxconnections";

constexpr const char *AppHomePath = "app_homepath";

constexpr const char *MetricPort = "metric_port";
constexpr const char *MetricHost = "metric_host";
constexpr const char *MetricFlavor = "metric_flavor";

}
