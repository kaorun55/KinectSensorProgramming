#include <iostream>
#include <stdexcept>

#include <XnCppWrapper.h>
#include <XnVSessionManager.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#elif (XN_PLATFORM == XN_PLATFORM_MACOSX)
const char* CONFIG_XML_PATH = "../../../../../Data/SamplesConfig.xml";
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#else
const char* CONFIG_XML_PATH = "Data/SamplesConfig.xml";
#endif

int main (int argc, char * const argv[]) {
    try {
        /* OpenNIのコンテキストを初期化する */
        xn::Context context;
        XnStatus rc = context.InitFromXmlFile(CONFIG_XML_PATH);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }        
        
        // NITEのセッションマネージャーを初期化する
        XnVSessionManager sessionManager;
        rc = sessionManager.Initialize(&context, "Wave,Click", "RaiseHand");
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }

        std::cout << "Success" << std::endl;
    }
    catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    return 0;
}
