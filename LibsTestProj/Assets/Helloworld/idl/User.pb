import "UserInfo.pb";

message User{
	required int32 id  = 1;
	repeated int32 status  = 2;
	required string pwdMd5  = 3;
	required string regTime  = 4;
	required UserInfo info = 5;
}

