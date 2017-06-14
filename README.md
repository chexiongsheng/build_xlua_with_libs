xLua常用库集成
=====

## PBC

云风的protobuf实现。

## LuaSocket

xLua默认集成库。

## RapidJson

json处理，特点是Rapid。

## LPeg

模式匹配库。

## 如何使用

* 用lua53版本的用build/plugin_lua53下的Plugins覆盖Unity工程Assets下对应的目录，注意是覆盖，别删除后拷贝，如果你要用luajit，用build/plugin_luajit下的
* 把LibsTestProj/Assets下的BuildInInit.cs和Resources目录放到Unity工程Assets下
* 库的初始化看实例：LibsTestProj/Assets/Helloworld/Helloworld.cs

## 注意

* 这只是常用库的扩展，首先你得安装了xLua
* 这几个库的使用请查阅相关文档
* 由于时间的关系，这不一定能做到和xLua版本总是同步，如果xLua报“wrong lib version ...”信息的话，请把https://github.com/Tencent/xLua/tree/master/build下的除CMakeLists.txt之外的文件目录，覆盖到https://github.com/chexiongsheng/build_xlua_with_libs/tree/master/build目录。