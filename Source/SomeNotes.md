
## FString
- 字面量构建
`FString MyFString = FString(TEXT("Hello"));`
- 格式化构建 
`FString MyFString = FString::Printf(TEXT("%s,%d"), *TestFString, 2333);`


## 获取枚举对应字符串
```C++
const UEnum* EnumObj = FindObject<UEnum>(ANY_PACKAGE, TEXT("EWeaponType"), true);
FString WeaponTypeStr = *EnumObj->GetDisplayNameTextByIndex(static_cast<uint8>(WeaponType)).ToString();
FString Info = FString::Printf(TEXT("Picked %s Ammo %d"), *WeaponTypeStr, AmmoAmount);
```


## debug信息输出到屏幕上
`GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("This is a debug message!"));`
-1使消息显示在屏幕上但不会替换现有消息
5.f 是持续时间
FColor字体颜色
然后传入一个FString就可以了