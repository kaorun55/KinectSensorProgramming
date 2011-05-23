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

void XN_CALLBACK_TYPE CroppingChange(xn::ProductionNode &node, void *pCookie)
{
    std::cout << "StateChangedHandler" << std::endl;
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
        
        XnCropping cropping = { 0 };
        image.GetCroppingCap().GetCropping(cropping);
        std::cout << cropping.bEnabled << "," << 
        cropping.nXOffset << "," << 
        cropping.nYOffset << "," << 
        cropping.nXSize << "," << 
        cropping.nYSize << "," << 
        std::endl;
        
        XnCallbackHandle hCallback;
        image.GetCroppingCap().RegisterToCroppingChange(CroppingChange,0,hCallback);
        
        cropping.bEnabled = FALSE;
        cropping.nXOffset = 0;
        cropping.nYOffset = 0;
        cropping.nXSize = 200;
        cropping.nYSize = 300;
        
        
        
        // カメラサイズのイメージを作成(8bitのRGB)
        XnMapOutputMode outputMode;
        image.GetMapOutputMode(outputMode);
        camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
                                 IPL_DEPTH_8U, 3);
        if (!camera) {
            throw std::runtime_error("error : cvCreateImage");
        }

        // メインループ
        while (1) {
            // カメライメージの更新を待ち、画像データを取得する
            context.WaitOneUpdateAll(image);
            
            xn::ImageMetaData imageMD;
            image.GetMetaData(imageMD);
            
            // カメラ画像の表示
            for (XnUInt y = imageMD.YOffset(); y < (imageMD.YOffset() + imageMD.YRes()); ++y) {
                const XnUInt8*src = imageMD.Data() + (imageMD.XRes() * y * 3);
                char* dest = camera->imageData + (camera->width * y * 3);
                for (XnUInt x = imageMD.XOffset(); x < (imageMD.XOffset() + imageMD.XRes()); ++x, src += 3, dest += 3) {
                    dest[0] = src[0];
                    dest[1] = src[1];
                    dest[2] = src[2];
                }
            }

            ::cvCvtColor(camera, camera, CV_BGR2RGB);
            ::cvShowImage("KinectImage", camera);
            
            // キーの取得
            char key = cvWaitKey(10);
            // 終了する
            if (key == 'q') {
                break;
            }
            // クロッピングの有効/無効を切り替える
            else if (key == 'c') {
                cropping.bEnabled = !cropping.bEnabled;
                image.GetCroppingCap().SetCropping(cropping);
                
                // 背景の初期化
                memset(camera->imageData, 255, camera->imageSize);
            }
        }
    }
    catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    ::cvReleaseImage(&camera);
    
    return 0;
}
