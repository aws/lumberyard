set (CMAKE_CXX_COMPILER_WORKS TRUE)
set (CMAKE_C_COMPILER_WORKS TRUE)
set (CMAKE_SYSTEM_NAME Darwin)
set (CMAKE_SYSTEM_VERSION 1)
set (UNIX True)
set (APPLE True)
set (IOS True)
set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")
set (CMAKE_FIND_ROOT_PATH /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS8.1.sdk ${CMAKE_PREFIX_PATH} CACHE string  "iOS find search path root")
set (CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS8.1.sdk CACHE PATH "Sysroot for iOS")
set (CMAKE_OSX_ARCHITECTURES arm64 CACHE PATH "Architectures for iOS")
set (CMAKE_FIND_FRAMEWORK FIRST)
set (CMAKE_SYSTEM_FRAMEWORK_PATH
/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS8.1.sdk/System/Library/Frameworks
/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS8.1.sdk/System/Library/PrivateFrameworks
/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS8.1.sdk/Developer/Library/Frameworks
)
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
