﻿/*
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
        luaenv.AddBuildin("pb", XLua.LuaDLL.Lua.LoadLuaProfobuf);
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
        ------------------------------------
        local pb = require 'pb'
        local protoc = require 'protoc'

        assert(protoc:load [[
        message Phone {
            optional string name        = 1;
            optional int64  phonenumber = 2;
        }
        message Person {
            optional string name     = 1;
            optional int32  age      = 2;
            optional string address  = 3;
            repeated Phone  contacts = 4;
        } ]])

        local data = {
        name = 'ilse',
        age  = 18,
            contacts = {
                { name = 'alice', phonenumber = 12312341234 },
                { name = 'bob',   phonenumber = 45645674567 }
            }
        }

        local bytes = assert(pb.encode('Person', data))
        print(pb.tohex(bytes))

        local data2 = assert(pb.decode('Person', bytes))
        print(data2.name)
        print(data2.age)
        print(data2.address)
        print(data2.contacts[1].name)
        print(data2.contacts[1].phonenumber)
        print(data2.contacts[2].name)
        print(data2.contacts[2].phonenumber)
        "
);
        luaenv.Dispose();
	}
	
	// Update is called once per frame
	void Update () {
	
	}
}
