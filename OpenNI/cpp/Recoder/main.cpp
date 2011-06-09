// ユーザーを記録対象に追加(未サポート:http://viml.nchc.org.tw/blog/paper_info.php?CLASS_ID=1&SUB_ID=1&PAPER_ID=221 )
// 圧縮方法(http://viml.nchc.org.tw/blog/paper_info.php?CLASS_ID=1&SUB_ID=1&PAPER_ID=221 )
//#define XN_CODEC_NULL				XN_CODEC_ID(0, 0, 0, 0)
//#define XN_CODEC_UNCOMPRESSED		XN_CODEC_ID('N','O','N','E')
//#define XN_CODEC_JPEG				XN_CODEC_ID('J','P','E','G')
//#define XN_CODEC_16Z				XN_CODEC_ID('1','6','z','P')
//#define XN_CODEC_16Z_EMB_TABLES	XN_CODEC_ID('1','6','z','T')
//#define XN_CODEC_8Z				XN_CODEC_ID('I','m','8','z')

#include <iostream>
#include <stdexcept>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
const char* RECORDE_PATH = "../../../Data/record.oni";
#elif (XN_PLATFORM == XN_PLATFORM_MACOSX)
const char* CONFIG_XML_PATH = "../../../../../Data/SamplesConfig.xml";
const char* RECORDE_PATH = "../../../../../Data/record.oni";
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
const char* RECORDE_PATH = "../../../Data/record.oni";
#else
const char* CONFIG_XML_PATH = "Data/SamplesConfig.xml";
const char* RECORDE_PATH = "Data/record.oni";
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
        
        // イメージジェネレータの作成
        xn::ImageGenerator image;
        rc = context.FindExistingNode(XN_NODE_TYPE_IMAGE, image);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        // デプスジェネレータの作成
        xn::DepthGenerator depth;
        rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depth);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        // デプスの座標をイメージに合わせる
        depth.GetAlternativeViewPointCap().SetViewPoint(image);
        
        // レコーダーの作成
        xn::Recorder recorder;
        rc = recorder.Create(context);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }

        // 記録設定
        rc = recorder.SetDestination(XN_RECORD_MEDIUM_FILE, RECORDE_PATH);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        // イメージを記録対象に追加
        rc = recorder.AddNodeToRecording(image, XN_CODEC_JPEG);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        // デプスを記録対象に追加
        rc = recorder.AddNodeToRecording(depth, XN_CODEC_UNCOMPRESSED);
        if (rc != XN_STATUS_OK) {
            std::cout << __LINE__ << std::endl;
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        // 記録開始(WaitOneUpdateAllのタイミングで記録される)
        rc = recorder.Record();
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
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
            context.WaitAndUpdateAll();
            xn::ImageMetaData imageMD;
            image.GetMetaData(imageMD);
            
            // カメラ画像の表示
            //  Kinectからの入力がRGBであるため、BGRに変換して表示する
            memcpy(camera->imageData, imageMD.RGB24Data(), camera->imageSize);
            ::cvCvtColor(camera, camera, CV_RGB2BGR);
            ::cvShowImage("KinectImage", camera);

            // キーイベント
            char key = cvWaitKey(10);
            // 'q'が押されたら終了する
            if (key == 'q') {
                break;
            }
        }
    }
    catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    ::cvReleaseImage(&camera);
    
    return 0;
}
