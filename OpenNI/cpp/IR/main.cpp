#include <iostream>
#include <stdexcept>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#ifdef WIN32
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#else
const char* CONFIG_XML_PATH = "../../../../../../Data/SamplesConfig.xml";
#endif

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
        
        xn::IRGenerator ir;
        rc = context.FindExistingNode(XN_NODE_TYPE_IR, ir);
        if (rc != XN_STATUS_OK) {
            rc = ir.Create(context );
            if (rc != XN_STATUS_OK) {
                throw std::runtime_error(xnGetStatusString(rc));
            }
            
            XnMapOutputMode outputmode = { 640, 480, 30 };
            ir.SetMapOutputMode(outputmode);
        }

        {
            xn::IRMetaData  irMD;
            ir.GetMetaData(irMD);
            
            std::cout << "(" << irMD.XRes() << "," << irMD.YRes() << ")" << std::endl;
        }
        
        // カメラサイズのイメージを作成(8bitのRGB)
        XnMapOutputMode outputMode;
        ir.GetMapOutputMode(outputMode);
        camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
                                 IPL_DEPTH_16U, 1);
        if (!camera) {
            throw std::runtime_error("error : cvCreateImage");
        }
        
        // メインループ
        while (1) {
            if (ir.IsDataNew()){
                xn::IRMetaData  irMD;
                ir.GetMetaData(irMD);
                
                memcpy(camera->imageData, irMD.Data(), camera->imageSize);
            }

            ::cvShowImage("KinectImage", camera);
            
            // 終了する
            char key = cvWaitKey(10);
            if (key == 'q') {
                break;
            }
            // 反転する
            else if (key == 'm') {
                context.SetGlobalMirror(!context.GetGlobalMirror());
            }
        }
    }
    catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    ::cvReleaseImage(&camera);
    
    return 0;
}
