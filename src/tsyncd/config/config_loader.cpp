/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/
#include "config_loader.hpp"
#include "score_time/utils/string_parser.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>

namespace tsyncd
{
    namespace
    {
        static inline void ltrim(std::string &s)
        {
            std::size_t i = 0;
            while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
                ++i;
            s.erase(0, i);
        }

        static inline void rtrim(std::string &s)
        {
            if (s.empty())
                return;
            std::size_t i = s.size();
            while (i > 0 && std::isspace(static_cast<unsigned char>(s[i - 1])))
                --i;
            s.erase(i);
        }

        static inline void trim(std::string &s)
        {
            ltrim(s);
            rtrim(s);
        }

        static bool iequals(const std::string &a, const std::string &b)
        {
            if (a.size() != b.size())
                return false;
            for (std::size_t i = 0; i < a.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(a[i])) !=
                    std::tolower(static_cast<unsigned char>(b[i])))
                    return false;
            }
            return true;
        }

        static bool parse_bool(const std::string &val, bool &out)
        {
            if (val.empty())
                return false;

            if (val == "0")
            {
                out = false;
                return true;
            }
            if (val == "1")
            {
                out = true;
                return true;
            }

            if (iequals(val, "true") || iequals(val, "yes") || iequals(val, "on"))
            {
                out = true;
                return true;
            }
            if (iequals(val, "false") || iequals(val, "no") || iequals(val, "off"))
            {
                out = false;
                return true;
            }

            return false;
        }

        static void split_by_comma(const std::string &val, std::vector<std::string> &out)
        {
            out.clear();
            std::size_t start = 0;
            while (start < val.size())
            {
                std::size_t pos = val.find(',', start);
                std::string token;
                if (pos == std::string::npos)
                {
                    token = val.substr(start);
                    start = val.size();
                }
                else
                {
                    token = val.substr(start, pos - start);
                    start = pos + 1;
                }
                trim(token);
                if (!token.empty())
                    out.push_back(token);
            }
        }

        static bool parse_abs_mode(const std::string &val, EngineOptions::AbsMode &out)
        {
            if (val.empty())
                return false;

            bool is_num = true;
            for (char c : val)
            {
                if (!std::isdigit(static_cast<unsigned char>(c)) && c != '+' && c != '-')
                {
                    is_num = false;
                    break;
                }
            }

            if (is_num)
            {
                int v = 0;
                if (!score_time::utils::ParseInteger(val, v))
                {
                    return false;
                }
                if (v == 0)
                {
                    out = EngineOptions::AbsMode::kPublishOnly;
                    return true;
                }
                if (v == 1)
                {
                    out = EngineOptions::AbsMode::kDisciplineSystemClock;
                    return true;
                }
                return false;
            }

            if (iequals(val, "publish_only") || iequals(val, "publish-only"))
            {
                out = EngineOptions::AbsMode::kPublishOnly;
                return true;
            }
            if (iequals(val, "discipline") ||
                iequals(val, "discipline_system_clock") ||
                iequals(val, "discipline-system-clock"))
            {
                out = EngineOptions::AbsMode::kDisciplineSystemClock;
                return true;
            }

            return false;
        }

        static void apply_kv(const std::string &key, const std::string &val, EngineOptions &opt)
        {
            if (key == "iface_name")
            {
                opt.iface_name = val;
            }
            else if (key == "phc_device")
            {
                opt.phc_device = val;
            }
            else if (key == "shm_name")
            {
                opt.shm_name = val;
            }
            else if (key == "shm_size")
            {
                std::uint64_t size_val;
                if (!score_time::utils::ParseInteger(val, size_val))
                {
                    std::cerr << "[WARN] invalid shm_size: " << val << std::endl;
                    return;
                }
                opt.shm_size = static_cast<std::size_t>(size_val);
            }
            else if (key == "abs_mode")
            {
                EngineOptions::AbsMode m;
                if (parse_abs_mode(val, m))
                    opt.abs_mode = m;
                else
                    std::cerr << "[WARN] invalid abs_mode: " << val << std::endl;
            }
            else if (key == "ntp_servers")
            {
                std::vector<std::string> v;
                split_by_comma(val, v);
                if (!v.empty())
                    opt.ntp_servers = v;
            }
            else if (key == "ntp_port")
            {
                int port_val;
                if (!score_time::utils::ParseInteger(val, port_val) || port_val < 0 || port_val > 65535)
                {
                    std::cerr << "[WARN] invalid ntp_port: " << val << std::endl;
                    return;
                }
                opt.ntp_port = port_val;
            }
            else if (key == "ntp_query_interval_ms")
            {
                int interval_val;
                if (!score_time::utils::ParseInteger(val, interval_val) || interval_val < 0)
                {
                    std::cerr << "[WARN] invalid ntp_query_interval_ms: " << val << std::endl;
                    return;
                }
                opt.ntp_query_interval_ms = interval_val;
            }
            else if (key == "ntp_request_timeout_ms" || key == "ntp_timeout_ms")
            {
                int timeout_val;
                if (!score_time::utils::ParseInteger(val, timeout_val) || timeout_val < 0)
                {
                    std::cerr << "[WARN] invalid ntp_request_timeout_ms: " << val << std::endl;
                    return;
                }
                opt.ntp_request_timeout_ms = timeout_val;
            }
            else if (key == "ntp_samples_to_lock")
            {
                int samples_val;
                if (!score_time::utils::ParseInteger(val, samples_val) || samples_val < 0)
                {
                    std::cerr << "[WARN] invalid ntp_samples_to_lock: " << val << std::endl;
                    return;
                }
                opt.ntp_samples_to_lock = samples_val;
            }
            else if (key == "ntp_offset_ewma_alpha")
            {
                double alpha_val;
                if (!score_time::utils::ParseDouble(val, alpha_val) || alpha_val < 0.0 || alpha_val > 1.0)
                {
                    std::cerr << "[WARN] invalid ntp_offset_ewma_alpha: " << val << std::endl;
                    return;
                }
                opt.ntp_offset_ewma_alpha = alpha_val;
            }
            else if (key == "ntp_jitter_ewma_alpha")
            {
                double alpha_val;
                if (!score_time::utils::ParseDouble(val, alpha_val) || alpha_val < 0.0 || alpha_val > 1.0)
                {
                    std::cerr << "[WARN] invalid ntp_jitter_ewma_alpha: " << val << std::endl;
                    return;
                }
                opt.ntp_jitter_ewma_alpha = alpha_val;
            }
            else if (key == "abs_publish_interval_ms")
            {
                int interval_val;
                if (!score_time::utils::ParseInteger(val, interval_val) || interval_val < 0)
                {
                    std::cerr << "[WARN] invalid abs_publish_interval_ms: " << val << std::endl;
                    return;
                }
                opt.abs_publish_interval_ms = interval_val;
            }
            else if (key == "abs_external_enable")
            {
                bool b{};
                if (parse_bool(val, b))
                    opt.abs_external_enable = b;
                else
                    std::cerr << "[WARN] invalid abs_external_enable: " << val << std::endl;
            }
            else if (key == "abs_source_socket")
            {
                opt.abs_source_socket = val;
            }
            else if (key == "abs_source_timeout_ms" || key == "abs_timeout_ms")
            {
                int timeout_val;
                if (!score_time::utils::ParseInteger(val, timeout_val) || timeout_val < 0)
                {
                    std::cerr << "[WARN] invalid abs_source_timeout_ms: " << val << std::endl;
                    return;
                }
                opt.abs_source_timeout_ms = timeout_val;
            }
            else if (key == "pdelay_req_interval_ms" || key == "pdelay_cycle")
            {
                int interval_val;
                if (!score_time::utils::ParseInteger(val, interval_val) || interval_val < 0)
                {
                    std::cerr << "[WARN] invalid pdelay_req_interval_ms: " << val << std::endl;
                    return;
                }
                opt.pdelay_req_interval_ms = interval_val;
            }
            else if (key == "pdelay_timeout_ms")
            {
                int timeout_val;
                if (!score_time::utils::ParseInteger(val, timeout_val) || timeout_val < 0)
                {
                    std::cerr << "[WARN] invalid pdelay_timeout_ms: " << val << std::endl;
                    return;
                }
                opt.pdelay_timeout_ms = timeout_val;
            }
            else if (key == "sync_timeout_ms")
            {
                int timeout_val;
                if (!score_time::utils::ParseInteger(val, timeout_val) || timeout_val < 0)
                {
                    std::cerr << "[WARN] invalid sync_timeout_ms: " << val << std::endl;
                    return;
                }
                opt.sync_timeout_ms = timeout_val;
            }
            else if (key == "unstable_offset_threshold_ns")
            {
                std::int64_t threshold_val;
                if (!score_time::utils::ParseInteger(val, threshold_val))
                {
                    std::cerr << "[WARN] invalid unstable_offset_threshold_ns: " << val << std::endl;
                    return;
                }
                opt.unstable_offset_threshold_ns = threshold_val;
            }
            else if (key == "jump_future_threshold_ns")
            {
                std::int64_t threshold_val;
                if (!score_time::utils::ParseInteger(val, threshold_val))
                {
                    std::cerr << "[WARN] invalid jump_future_threshold_ns: " << val << std::endl;
                    return;
                }
                opt.jump_future_threshold_ns = threshold_val;
            }
            else
            {
                std::cerr << "[INFO] ignore unknown config key: " << key << std::endl;
            }
        }

    } // namespace

    bool LoadEngineOptionsFromFile(const std::string &path, EngineOptions &opt)
    {
        std::ifstream in(path);
        if (!in.is_open())
        {
            std::cerr << "[WARN] cannot open config file: " << path << std::endl;
            return false;
        }

        std::string line;
        std::string section;
        int line_no = 0;

        while (std::getline(in, line))
        {
            ++line_no;
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            std::string raw = line;
            trim(line);

            if (line.empty())
                continue;

            if (line[0] == '#' || line[0] == ';')
                continue;

            if (line.front() == '[' && line.back() == ']')
            {
                section = line.substr(1, line.size() - 2);
                trim(section);
                continue;
            }

            auto pos = line.find('=');
            if (pos == std::string::npos)
            {
                std::cerr << "[WARN] invalid line (no '=') at " << line_no << ": " << raw << std::endl;
                continue;
            }

            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            trim(key);
            trim(val);
            if (key.empty())
            {
                std::cerr << "[WARN] empty key at line " << line_no << std::endl;
                continue;
            }

            apply_kv(key, val, opt);
        }

        return true;
    }

} // namespace tsyncd
