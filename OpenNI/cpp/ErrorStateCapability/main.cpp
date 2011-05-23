#include <iostream>
#include <stdexcept>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#ifdef WIN32
const char* CONFIG_XML_PATH = "../../../../../Data/SamplesConfig.xml";
#else
const char* CONFIG_XML_PATH = "../../SamplesConfig.xml";
#endif

void XN_CALLBACK_TYPE ErrorChange(xn::ProductionNode &node, void *pCookie)
{
    std::cout << "ErrorChange:" << std::endl;
}

int main (int argc, char * argv[])
{
    IplImage* camera = 0;
    
    try {
        // コンテキストの初期化
        xn::Context context;
        XnStatus rc = context.InitFromXmlFile(CONFIG_XML_PATH);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        // イメージジェネレータの作成
        xn::ImageGenerator image;
        rc = context.FindExistingNode(XN_NODE_TYPE_IMAGE, image);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        // エラー状態が変わると通知されるコールバックの登録
        XnCallbackHandle hCallback;
        xn::ErrorStateCapability error = image.GetErrorStateCap();
        error.RegisterToErrorStateChange(ErrorChange, 0, hCallback);

        // わざとエラーにする
        rc = image.AddNeededNode(image);
        if (rc == error.GetErrorState()) {
            std::cout << "same" << std::endl;
        }
        
        if (rc != XN_STATUS_OK) {
            std::cout << "failed" << std::endl;
            throw std::runtime_error(xnGetStatusString(rc));
        }
        std::cout << "success" << std::endl;
    }
    catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    ::cvReleaseImage(&camera);
    
    return 0;
}
