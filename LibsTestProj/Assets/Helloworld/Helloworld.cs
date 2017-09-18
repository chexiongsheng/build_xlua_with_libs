/*
 * Tencent is pleased to support the open source community by making xLua available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

using UnityEngine;
using XLua;
using System.Collections.Generic;
using System;

public static class GenCfg
{
    [LuaCallCSharp]
    static List<Type> cfg = new List<Type>()
    {
        typeof(TextAsset)
    };
}

public class Helloworld : MonoBehaviour {
	// Use this for initialization
	void Start () {
        LuaEnv luaenv = new LuaEnv();
        luaenv.AddBuildin("rapidjson", XLua.LuaDLL.Lua.LoadRapidJson);
        luaenv.AddBuildin("lpeg", XLua.LuaDLL.Lua.LoadLpeg);
        luaenv.AddBuildin("protobuf.c", XLua.LuaDLL.Lua.LoadProtobufC);
        luaenv.DoString(@"
        ------------------------------------
        local rapidjson = require 'rapidjson' 
        local t = rapidjson.decode('{""a"":123}')
        print(t.a)
        t.a = 456
        local s = rapidjson.encode(t)
        print('json', s)
        ------------------------------------
        local lpeg = require 'lpeg'
        print(lpeg.match(lpeg.R '09','123'))
"
//webgl下，pbc register时就报out of memery，解决前例子先去掉
#if !UNITY_WEBGL
+ @"        ------------------------------------
        local protobuf = require 'protobuf'
        protobuf.register(CS.UnityEngine.Resources.Load('proto/UserInfo.pb').bytes)
        protobuf.register(CS.UnityEngine.Resources.Load('proto/User.pb').bytes)

        local userInfo = {}
        userInfo.name = 'world'
        userInfo.diamond = 998
        userInfo.level = 100

        local user = { }
        user.id = 1
        user.status = { 1,0,2,4}
        user.pwdMd5 = 'md5'
        user.regTime = '2017-03-29 12:00:00'
        user.info = userInfo

        --序列化
        local encode = protobuf.encode('User', user)

        -- 反序列化
        local user_decode = protobuf.decode('User', encode)

        assert(user.id == user_decode.id and user.info.diamond == user_decode.info.diamond)
        print('hello', user_decode.info.name)

        "
#endif
);
        luaenv.Dispose();
	}
	
	// Update is called once per frame
	void Update () {
	
	}
}
