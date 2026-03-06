#!/usr/bin/env python3
# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
"""Bazel credential helper for *.qnx.com downloads.

Reads credentials from SCORE_QNX_USER / SCORE_QNX_PASSWORD environment
variables, or from ~/.netrc (machine qnx.com).

Usage in .bazelrc:
  build --credential_helper=*.qnx.com=%workspace%/scripts/internal/qnx_creds.py
"""

import http.cookiejar
import json
import netrc
import os
import sys
import urllib.parse
import urllib.request


def get_credentials():
    user = os.environ.get("SCORE_QNX_USER", "")
    password = os.environ.get("SCORE_QNX_PASSWORD", "")
    if user and password:
        return user, password
    try:
        nrc = netrc.netrc()
        auth = nrc.authenticators("qnx.com")
        if auth:
            return auth[0], auth[2]
    except Exception:
        pass
    return None, None


def get_myqnx_cookie(user, password):
    data = urllib.parse.urlencode(
        {
            "userlogin": user,
            "password": password,
            "x": "1",
            "y": "1",
        }
    ).encode("ascii")
    jar = http.cookiejar.CookieJar()
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))
    opener.open("https://www.qnx.com/account/login.html", data)
    for cookie in jar:
        if cookie.name == "myQNX":
            return cookie.value
    return None


def main():
    _ = json.load(sys.stdin)  # consume the request (uri is not needed)
    user, password = get_credentials()
    if not user or not password:
        sys.stderr.write(
            "qnx_creds.py: no credentials found. "
            "Set SCORE_QNX_USER and SCORE_QNX_PASSWORD, "
            "or add 'machine qnx.com login <u> password <p>' to ~/.netrc\n"
        )
        sys.exit(1)
    cookie = get_myqnx_cookie(user, password)
    if not cookie:
        sys.stderr.write("qnx_creds.py: login failed, no myQNX cookie returned\n")
        sys.exit(1)
    print(json.dumps({"headers": {"Cookie": [f"myQNX={cookie}"]}}))


if __name__ == "__main__":
    main()
