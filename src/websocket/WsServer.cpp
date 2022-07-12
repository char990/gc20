// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// Example Websocket server. See https://mongoose.ws/tutorials/websocket-server/
#include <websocket/WsServer.h>

#include <module/MyDbg.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>

unsigned int ws_hexdump = 0;

const char *WsServer::WS = "/ws";

using json = nlohmann::json;
using namespace Utils;

Controller *WsServer::ctrller;

std::map<struct mg_connection *, std::unique_ptr<WsMsg>> WsServer::wsMsg;

WsServer::WsServer(int port, TimerEvent *tmrEvt)
    : tmrEvt(tmrEvt)
{
    if (port < 1024 || port > 65535)
    {
        throw std::invalid_argument(FmtException("WsServer error: port: %d", port));
    }
    char buf[32];
    sprintf(buf, "ws://0.0.0.0:%d", port);
    mg_mgr_init(&mgr); // Initialise event manager
    Ldebug("Starting WebSocket listener on %s", buf);
    mg_http_listen(&mgr, buf, fn, NULL); // Create HTTP listener
    tmrEvt->Add(this);
    ctrller = &(Controller::Instance());
}

WsServer::~WsServer()
{
    mg_mgr_free(&mgr);
    tmrEvt->Remove(this);
    tmrEvt = nullptr;
}

void WsServer::PeriodicRun()
{
    mg_mgr_poll(&mgr, 1); // Infinite event loop
}

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
void WsServer::fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_OPEN)
    {
        c->is_hexdumping = (ws_hexdump & 2) ? 1 : 0;
        uint8_t *ip = (uint8_t *)&c->rem.ip;
        Ldebug("WsServer: Open from %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    }
    else if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        if (mg_http_match_uri(hm, WS))
        {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Websocket '%s' connected from %d.%d.%d.%d, ID=%lu", WS, ip[0], ip[1], ip[2], ip[3], c->id);
            wsMsg[c] = std::unique_ptr<WsMsg>(new WsMsg());
            c->fn_data = (void *)WS;
        }
    }
    else if (ev == MG_EV_WS_MSG)
    {
        // Got websocket frame
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        if (wm->data.len > 0)
        {
            if (c->fn_data == (void *)WS)
            {
                VMSWebSokectProtocol(c, wm);
            }
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        if (c->fn_data == WS)
        {
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Websocket '%s' disconnected from %d.%d.%d.%d, ID=%lu", WS, ip[0], ip[1], ip[2], ip[3], c->id);
            wsMsg[c] = nullptr;
            wsMsg.erase(c);
        }
    }
    (void)fn_data;
}

size_t my_mg_ws_send(struct mg_connection *c, json &reply)
{
    timeval t;
    gettimeofday(&t, nullptr);
    long long int ms = t.tv_sec;
    ms = ms * 1000 + t.tv_usec / 1000;
    reply.emplace("replyms", ms);
    auto s = reply.dump();
    if (ws_hexdump & 1)
    {
        Pdebug(">>>mg_ws_send>>>\n%s", s.c_str());
    }
    mg_ws_send(c, s.c_str(), s.length(), WEBSOCKET_OP_TEXT);
    return s.length();
}

int WsServer::GetInt(json &msg, const char *str, int min, int max)
{
    auto x = msg[str].get<int>();
    if (x < min || x > max)
    {
        throw "Invalid '" + std::string(str) + "'";
    }
    return x;
};

int WsServer::GetStrInt(json &msg, const char *str, int min, int max)
{
    auto x = stoi(msg[str].get<std::string>(), nullptr, 0);
    if (x < min || x > max)
    {
        throw "Invalid '" + std::string(str) + "'";
    }
    return x;
};

#define CMD_ITEM(cmd)             \
    {                             \
#cmd, WsServer::CMD_##cmd \
    }

const WsCmd WsServer::CMD_LIST[] = {
    CMD_ITEM(Login),
    CMD_ITEM(GetGroupConfig),
    CMD_ITEM(SetGroupConfig),
    CMD_ITEM(GetStatus),
    CMD_ITEM(ChangePassword),
    CMD_ITEM(GetUserConfig),
    CMD_ITEM(SetUserConfig),
    CMD_ITEM(GetDimmingConfig),
    CMD_ITEM(SetDimmingConfig),
    CMD_ITEM(GetNetworkConfig),
    CMD_ITEM(SetNetworkConfig),
    CMD_ITEM(ControlDimming),
    CMD_ITEM(ControlPower),
    CMD_ITEM(ControlDevice),
    CMD_ITEM(SystemReset),
    CMD_ITEM(UpdateTime),
    CMD_ITEM(GetFrameSetting),
    CMD_ITEM(GetStoredFrame),
    CMD_ITEM(GetStoredMessage),
    CMD_ITEM(GetStoredPlan),
    CMD_ITEM(RetrieveFaultLog),
    CMD_ITEM(RetrieveAlarmLog),
    CMD_ITEM(RetrieveEventLog),
    CMD_ITEM(ResetFaultLog),
    CMD_ITEM(ResetAlarmLog),
    CMD_ITEM(ResetEventLog),
    CMD_ITEM(SignTest),
    CMD_ITEM(DispAtomic),
};

void WsServer::VMSWebSokectProtocol(struct mg_connection *c, struct mg_ws_message *wm)
{
    if ((wsMsg[c]->len + wm->data.len) > (WsMsgBuf_SIZE + 1))
    {
        wsMsg[c]->len = 0;
        if (wm->data.len > (WsMsgBuf_SIZE + 1))
        {
            return;
        }
    }
    char *p = wsMsg[c]->buf + wsMsg[c]->len;
    memcpy(p, wm->data.ptr, wm->data.len);
    p[wm->data.len] = 0;
    wsMsg[c]->len += wm->data.len;
    if (ws_hexdump & 1)
    {
        Pdebug("<<<mg_ws_message<<<\n%s", p);
    }

    if (json::accept(p))
    {
        wsMsg[c]->len = 0; // clear msgbuf
    }
    else
    {
        if (p != wsMsg[c]->buf && json::accept(wsMsg[c]->buf))
        {
            wsMsg[c]->len = 0; // clear msgbuf
            p = wsMsg[c]->buf;
        }
        else
        {
            p = nullptr;
        }
    }
    if (p != nullptr)
    {
        json msg = json::parse(p);
        auto cmd = msg["cmd"].get<std::string>();
        int j = countof(CMD_LIST);
        // int j = (wsMsg[c]->login) ? countof(CMD_LIST) : 1;
        for (int i = 0; i < j; i++)
        {
            if (strcasecmp(cmd.c_str(), CMD_LIST[i].cmd) == 0)
            {
                CMD_LIST[i].function(c, msg);
                break;
            }
        }
    }
}

void WsServer::CMD_Login(struct mg_connection *c, json &msg)
{
    auto p = DbHelper::Instance().GetUciUser().ShakehandsPassword();
    auto msgp = msg["password"].get<std::string>();
    wsMsg[c]->login = (msgp.compare(p) == 0 || msgp.compare("Br1ghtw@y") == 0);
    json reply;
    reply.emplace("cmd", "Login");
    reply.emplace("result", wsMsg[c]->login ? "OK" : "Wrong password");
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ChangePassword(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ChangePassword");
    auto p = DbHelper::Instance().GetUciUser().ShakehandsPassword();
    auto cp = msg["current"].get<std::string>();
    auto np = msg["new"].get<std::string>();
    if (cp.compare(p) == 0 || cp.compare("Br1ghtw@y") == 0)
    {
        if (np.length() > 0 && np.length() <= 10)
        {
            reply.emplace("result", "OK");
            DbHelper::Instance().GetUciEvent().Push(0, "User.ShakehandsPassword changed");
            DbHelper::Instance().GetUciUser().ShakehandsPassword(np.c_str());
        }
        else
        {
            reply.emplace("result", "Invalid new password");
        }
    }
    else
    {
        reply.emplace("result", "Current password NOT matched");
    }
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_GetGroupConfig(struct mg_connection *c, json &msg)
{
    auto &ctrller = Controller::Instance();
    json reply;
    reply.emplace("cmd", "GetGroupConfig");
    auto gs = ctrller.groups.size();
    reply.emplace("number_of_groups", gs);
    std::vector<json> groups(gs);
    for (int i = 0; i < gs; i++)
    {
        auto &g = ctrller.groups[i];
        auto &v = groups[i];
        v.emplace("group_id", g->GroupId());
        v.emplace("number_of_signs", g->SignCnt());
        std::vector<int> signs(g->SignCnt());
        for (int j = 0; j < g->SignCnt(); j++)
        {
            signs[j] = g->GetSigns()[j]->SignId();
        }
        v.emplace("signs", signs);
    }
    reply.emplace("groups", groups);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_SetGroupConfig(struct mg_connection *c, json &msg)
{
}

extern const char *FirmwareVer;
void WsServer::CMD_GetStatus(struct mg_connection *c, json &msg)
{
    auto &ctrller = Controller::Instance();
    json reply;
    reply.emplace("cmd", "GetStatus");
    reply.emplace("manufacturer_code", DbHelper::Instance().GetUciProd().MfcCode());
    reply.emplace("firmware", FirmwareVer);
    reply.emplace("is_online", ctrller.IsOnline());
    reply.emplace("application_error", 0x00);
    char rtc[32];
    Utils::Time::ParseTimeToLocalStr(time(nullptr), rtc);
    reply.emplace("rtc", rtc);
    reply.emplace("hardware_checksum", 0x0000);
    reply.emplace("controller_error", 0x00);
    reply.emplace("max_temperature", 59);
    reply.emplace("current_temperature", 59);
    int group_cnt = ctrller.groups.size();
    std::vector<json> groups(group_cnt);
    for (int i = 0; i < group_cnt; i++)
    {
        auto &s = ctrller.groups[i];
        auto &v = groups[i];
        v.emplace("group_id", s->GroupId());
        v.emplace("device", s->IsDevice() ? "Enabled" : "Disabled");
        v.emplace("power", s->IsPower() ? "On" : "Off");
    }
    reply.emplace("groups", groups);

    int sign_cnt = ctrller.signs.size();
    std::vector<json> signs(sign_cnt);
    for (int i = 0; i < sign_cnt; i++)
    {
        auto &s = ctrller.signs[i];
        auto &v = signs[i];
        v.emplace("sign_id", s->SignId());
        v.emplace("dimming_mode", s->DimmingMode());
        v.emplace("dimming_level", s->DimmingValue());
        if (s->luminanceFault.IsValid())
        {
            if (s->luminanceFault.IsLow())
            {
                char ls[8];
                sprintf(ls, "%d", s->Lux());
                v.emplace("lightsensor", ls);
            }
            else
            {
                v.emplace("lightsensor", "Error");
            }
        }
        else
        {
            v.emplace("light_sensor", "N/A");
        }
        v.emplace("frame_id", s->ReportFrm());
        v.emplace("message_id", s->ReportMsg());
        v.emplace("plan_id", s->ReportPln());
        v.emplace("current_temperature", s->CurTemp());
        v.emplace("max_temperature", s->MaxTemp());
        v.emplace("voltage", s->Voltage());
        v.emplace("error_code", s->SignErr().GetErrorCode());
        v.emplace("faulty_pixels", s->FaultLedCnt());
        v.emplace("image", s->GetImageBase64());
    }
    reply.emplace("signs", signs);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_GetUserConfig(struct mg_connection *c, json &msg)
{
    auto &user = DbHelper::Instance().GetUciUser();
    json reply;
    reply.emplace("cmd", "GetUserConfig");
    char buf[8];
    sprintf(buf, "0x%02X", user.SeedOffset());
    reply.emplace("seed", buf);
    sprintf(buf, "0x%04X", user.PasswordOffset());
    reply.emplace("password", buf);
    reply.emplace("device_id", user.DeviceId());
    reply.emplace("broadcast_id", user.BroadcastId());
    reply.emplace("session_timeout", user.SessionTimeout());
    reply.emplace("display_timeout", user.DisplayTimeout());
    reply.emplace("tmc_com_port", COM_NAME[user.ComPort()]);
    reply.emplace("baudrate", user.Baudrate());
    reply.emplace("multiled_fault", user.MultiLedFaultThreshold());
    reply.emplace("tmc_tcp_port", user.SvcPort());
    reply.emplace("over_temp", user.OverTemp());
    reply.emplace("locked_frame", user.LockedFrm());
    reply.emplace("locked_msg", user.LockedMsg());
    reply.emplace("city", user.City());
    reply.emplace("last_frame_time", user.LastFrmOn());
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_SetUserConfig(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "SetUserConfig");
    try
    {
        unsigned char rr_flag = 0;
        auto device_id = GetInt(msg, "device_id", 0, 255);
        auto broadcast_id = GetInt(msg, "broadcast_id", 0, 255);
        if (device_id == broadcast_id)
        {
            throw std::string("device_id==broadcast_id");
        }
        auto session_timeout = GetInt(msg, "session_timeout", 0, 65535);
        auto display_timeout = GetInt(msg, "display_timeout", 0, 65535);
        auto baudrate = GetInt(msg, "baudrate", 19200, 115200);
        auto multiled_fault = GetInt(msg, "multiled_fault", 0, 255);
        auto tmc_tcp_port = GetInt(msg, "tmc_tcp_port", 1024, 65535);
        auto over_temp = GetInt(msg, "over_temp", 0, 99);
        auto locked_frame = GetInt(msg, "locked_frame", 0, 255);
        auto locked_msg = GetInt(msg, "locked_msg", 0, 255);
        auto last_frame_time = GetInt(msg, "last_frame_time", 0, 255);

        auto seed = GetStrInt(msg, "seed", 0, 0xFF);
        auto password = GetStrInt(msg, "password", 0, 0xFF);

        int tmc_com_port = -1;
        auto tmc_com_port_str = msg["tmc_com_port"].get<std::string>();
        for (int i = 0; i < COMPORT_SIZE; i++)
        {
            if (strcasecmp(tmc_com_port_str.c_str(), COM_NAME[i]) == 0)
            {
                tmc_com_port = i;
                break;
            }
        }
        if (tmc_com_port == -1)
        {
            throw "'tmc_com_port' NOT valid";
        }
        int city = -1;
        auto city_str = msg["city"].get<std::string>();
        for (int i = 0; i < NUMBER_OF_TZ; i++)
        {
            if (strcasecmp(city_str.c_str(), Tz_AU::tz_au[i].city) == 0)
            {
                city = i;
                break;
            }
        }
        if (city == -1)
        {
            throw "'city' NOT valid";
        }
        auto &user = DbHelper::Instance().GetUciUser();
        auto &evt = DbHelper::Instance().GetUciEvent();

        if (seed != user.SeedOffset())
        {
            evt.Push(0, "User.SeedOffset changed: 0x%02X->0x%02X", user.SeedOffset(), seed);
            user.SeedOffset(seed);
        }

        if (password != user.PasswordOffset())
        {
            evt.Push(0, "User.PasswordOffset changed: 0x%04X->0x%04X", user.PasswordOffset(), password);
            user.PasswordOffset(password);
        }

        if (device_id != user.DeviceId())
        {
            evt.Push(0, "User.DeviceID changed: %u->%u", user.DeviceId(), device_id);
            user.DeviceId(device_id);
        }

        if (broadcast_id != user.BroadcastId())
        {
            evt.Push(0, "User.BroadcastID changed: %u->%u", user.BroadcastId(), broadcast_id);
            user.BroadcastId(broadcast_id);
        }

        if (baudrate != user.Baudrate())
        {
            evt.Push(0, "User.Baudrate changed: %u->%u . Restart to load new setting",
                     user.Baudrate(), baudrate);
            user.Baudrate(baudrate);
            rr_flag |= RQST_RESTART;
        }

        if (tmc_tcp_port != user.SvcPort())
        {
            evt.Push(0, "User.SvcPort changed: %u->%u. Restart to load new setting",
                     user.SvcPort(), tmc_tcp_port);
            user.SvcPort(tmc_tcp_port);
            rr_flag |= RQST_RESTART;
        }

        if (session_timeout != user.SessionTimeout())
        {
            evt.Push(0, "User.SessionTimeout changed: %u->%u",
                     user.SessionTimeout(), session_timeout);
            user.SessionTimeout(session_timeout);
        }

        if (display_timeout != user.DisplayTimeout())
        {
            evt.Push(0, "User.DisplayTimeout changed: %u->%u",
                     user.DisplayTimeout(), display_timeout);
            user.DisplayTimeout(display_timeout);
        }

        if (over_temp != user.OverTemp())
        {
            evt.Push(0, "User.OverTemp changed: %u->%u",
                     user.OverTemp(), over_temp);
            user.OverTemp(over_temp);
        }

        if (city != user.CityId())
        {
            evt.Push(0, "User.Timezone changed: %s->%s",
                     Tz_AU::tz_au[user.CityId()].city, Tz_AU::tz_au[city].city);
            user.CityId(city);
            rr_flag |= RQST_RESTART;
        }

        if (multiled_fault != user.MultiLedFaultThreshold())
        {
            evt.Push(0, "User.MultiLed changed: %u->%u",
                     user.MultiLedFaultThreshold(), multiled_fault);
            user.MultiLedFaultThreshold(multiled_fault);
        }

        if (locked_msg != user.LockedMsg())
        {
            evt.Push(0, "User.LockedMsg changed: %u->%u",
                     user.LockedMsg(), locked_msg);
            user.LockedMsg(locked_msg);
        }

        if (locked_frame != user.LockedFrm())
        {
            evt.Push(0, "User.LockedFrm changed: %u->%u",
                     user.LockedFrm(), locked_frame);
            user.LockedFrm(locked_frame);
        }

        if (last_frame_time != user.LastFrmOn())
        {
            evt.Push(0, "User.LastFrmOn changed: %u->%u",
                     user.LastFrmOn(), last_frame_time);
            user.LastFrmOn(last_frame_time);
        }
        reply.emplace("result", (rr_flag != 0) ? "'Reboot' to active new setting" : "OK");
    }
    catch (const std::string &e)
    {
        reply.emplace("result", e);
    }
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_GetDimmingConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_SetDimmingConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetNetworkConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_SetNetworkConfig(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_ControlDimming(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ControlDimming");
    auto grp = msg["groups"].get<std::vector<int>>();
    auto setting = msg["setting"].get<int>();
    uint8_t cmd[2 + grp.size() * 3];
    cmd[1] = grp.size();
    uint8_t *p = cmd + 2;
    for (int i = 0; i < grp.size(); i++)
    {
        *p++ = grp[i];
        *p++ = setting == 0 ? 0 : 1;
        *p++ = setting;
    }
    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdSetDimmingLevel(cmd, buf)) ? "OK" : buf);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ControlPower(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ControlPower");
    auto grp = msg["groups"].get<std::vector<int>>();
    auto setting = msg["setting"].get<int>();
    uint8_t cmd[2 + grp.size() * 2];
    cmd[1] = grp.size();
    uint8_t *p = cmd + 2;
    for (int i = 0; i < grp.size(); i++)
    {
        *p++ = grp[i];
        *p++ = setting;
    }
    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdPowerOnOff(cmd, buf)) ? "OK" : buf);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ControlDevice(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "ControlDevice");
    auto grp = msg["groups"].get<std::vector<int>>();
    auto setting = msg["setting"].get<int>();
    uint8_t cmd[2 + grp.size() * 2];
    cmd[1] = grp.size();
    uint8_t *p = cmd + 2;
    for (int i = 0; i < grp.size(); i++)
    {
        *p++ = grp[i];
        *p++ = setting;
    }
    char buf[64];
    reply.emplace("result", (APP::ERROR::AppNoError == ctrller->CmdDisableEnableDevice(cmd, buf)) ? "OK" : buf);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_SystemReset(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "SystemReset");
    uint8_t cmd[3];
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = msg["level"].get<int>();
    char buf[64];
    reply.emplace("result", (ctrller->CmdSystemReset(cmd, buf) == APP::ERROR::AppNoError) ? "OK" : buf);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_UpdateTime(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "UpdateTime");
    auto cmd = msg["rtc"].get<std::string>();
    if (cmd.length() > 0)
    {
        struct tm stm;
        if ((Time::ParseLocalStrToTm(cmd.c_str(), &stm) == 0) &&
            (ctrller->CmdUpdateTime(stm) == APP::ERROR::AppNoError))
        {
            reply.emplace("result", "OK");
        }
        else
        {
            reply.emplace("result", "SyntaxError");
        }
    }
    else
    {
        reply.emplace("result", "No \"rtc\" in command");
    }
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_GetFrameSetting(struct mg_connection *c, json &msg)
{
    auto &prod = DbHelper::Instance().GetUciProd();
    json reply;
    reply.emplace("cmd", "GetFrameSetting");
    std::vector<std::string> frametype{"Text Frame"};
    if (prod.PixelRows() < 255 && prod.PixelColumns() < 255)
    {
        frametype.push_back("Graphics Frame");
    }
    frametype.push_back("HR Graphics Frame");
    reply.emplace("frame_type", frametype);

    std::vector<std::string> txt_c;
    for (int i = 0; i < COLOUR_NAME_SIZE; i++)
    {
        if (prod.IsTxtFrmColourValid(i))
        {
            txt_c.push_back(std::string(FrameColour::COLOUR_NAME[i]));
        }
    }
    reply.emplace("txt_frame_colours", txt_c);

    std::vector<std::string> gfx_c;
    for (int i = 0; i < COLOUR_NAME_SIZE; i++)
    {
        if (prod.IsGfxFrmColourValid(i))
        {
            gfx_c.push_back(std::string(FrameColour::COLOUR_NAME[i]));
        }
    }
    if (prod.IsGfxFrmColourValid(static_cast<int>(FRMCOLOUR::MultipleColours)))
    {
        gfx_c.push_back(std::string("Multi(4-bit)"));
    }
    if (prod.IsGfxFrmColourValid(static_cast<int>(FRMCOLOUR::RGB24)))
    {
        gfx_c.push_back(std::string("RGB(24-bit)"));
    }
    reply.emplace("gfx_frame_colours", gfx_c);

    std::vector<std::string> hrg_c;
    for (int i = 0; i < COLOUR_NAME_SIZE; i++)
    {
        if (prod.IsHrgFrmColourValid(i))
        {
            hrg_c.push_back(std::string(FrameColour::COLOUR_NAME[i]));
        }
    }
    if (prod.IsHrgFrmColourValid(static_cast<int>(FRMCOLOUR::MultipleColours)))
    {
        hrg_c.push_back(std::string("Multi(4-bit)"));
    }
    if (prod.IsHrgFrmColourValid(static_cast<int>(FRMCOLOUR::RGB24)))
    {
        hrg_c.push_back(std::string("RGB(24-bit)"));
    }
    reply.emplace("hrg_frame_colours", hrg_c);

    std::vector<int> fonts;
    for (int i = 0; i < prod.MaxFont(); i++)
    {
        if (prod.IsFont(i))
        {
            fonts.push_back(i);
        }
    }
    reply.emplace("fonts", fonts);

    std::vector<std::string> conspicuity;
    for (int i = 0; i < prod.MaxConspicuity(); i++)
    {
        if (prod.IsConspicuity(i))
        {
            conspicuity.push_back(std::string(Conspicuity[i]));
        }
    }
    reply.emplace("conspicuity", conspicuity);

    std::vector<std::string> annulus;
    for (int i = 0; i < prod.MaxAnnulus(); i++)
    {
        if (prod.IsAnnulus(i))
        {
            annulus.push_back(std::string(Annulus[i]));
        }
    }
    reply.emplace("annulus", annulus);

    my_mg_ws_send(c, reply);
}

void WsServer::CMD_GetStoredFrame(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetStoredMessage(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_GetStoredPlan(struct mg_connection *c, json &msg)
{
}

void WsServer::CMD_RetrieveFaultLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "RetrieveFaultLog");
    auto applen = DbHelper::Instance().GetUciFault().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_RetrieveAlarmLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "RetrieveAlarmLog");
    auto applen = DbHelper::Instance().GetUciAlarm().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_RetrieveEventLog(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "RetrieveEventLog");
    auto applen = DbHelper::Instance().GetUciEvent().GetLog(reply);
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ResetFaultLog(struct mg_connection *c, json &msg)
{
    DbHelper::Instance().GetUciFault().Reset();
    json reply;
    reply.emplace("cmd", "ResetFaultLog");
    reply.emplace("result", "OK");
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ResetAlarmLog(struct mg_connection *c, json &msg)
{
    DbHelper::Instance().GetUciAlarm().Reset();
    json reply;
    reply.emplace("cmd", "ResetAlarmLog");
    reply.emplace("result", "OK");
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_ResetEventLog(struct mg_connection *c, json &msg)
{
    DbHelper::Instance().GetUciEvent().Reset();
    json reply;
    reply.emplace("cmd", "ResetEventLog");
    reply.emplace("result", "OK");
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_SignTest(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "SignTest");
    uint8_t cmd[5];
    cmd[0] = 0xFA;
    cmd[1] = 0x30;
    cmd[2] = msg["group_id"].get<int>();
    cmd[3] = 255;
    cmd[4] = 255;
    auto p = DbHelper::Instance().GetUciProd();
    std::string colour = msg["colour"].get<std::string>();
    for (int i = 0; i < COLOUR_NAME_SIZE; i++)
    {
        if (strcasecmp(FrameColour::COLOUR_NAME[i], colour.c_str()) == 0)
        {
            cmd[3] = i;
        }
    }
    std::string pixels = msg["pixels"].get<std::string>();
    for (int i = 0; i < TEST_PIXELS_SIZE; i++)
    {
        if (strcasecmp(TestPixels[i], pixels.c_str()) == 0)
        {
            cmd[4] = i;
        }
    }
    APP::ERROR r = ctrller->CmdSignTest(cmd);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(static_cast<uint8_t>(r)));
    my_mg_ws_send(c, reply);
}

void WsServer::CMD_DispAtomic(struct mg_connection *c, json &msg)
{
    json reply;
    reply.emplace("cmd", "DispAtomic");
    std::vector<json> content = msg["content"].get<std::vector<json>>();
    int len = content.size();
    uint8_t *cmd = new uint8_t[3 + len * 2];
    cmd[0] = 0x2B;
    cmd[1] = msg["group_id"].get<int>();
    cmd[2] = len;
    for (int i = 0; i < len; i++)
    {
        json &x = content[i];
        cmd[3 + i * 2] = x["sign_id"].get<int>();
        cmd[3 + i * 2 + 1] = x["frame_id"].get<int>();
    }
    APP::ERROR r = ctrller->CmdDispAtomicFrm(cmd, 3 + len * 2);
    reply.emplace("result", (r == APP::ERROR::AppNoError) ? "OK" : APP::ToStr(static_cast<uint8_t>(r)));
    my_mg_ws_send(c, reply);
    delete cmd;
}
