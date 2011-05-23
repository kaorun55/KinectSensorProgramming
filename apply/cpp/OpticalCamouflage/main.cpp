#include <iostream>
#include <stdexcept>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>

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

int main (int argc, char * argv[])
{
    IplImage* camera = 0;
    IplImage* background = 0;
    
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
        
        // ユーザーの作成
        xn::UserGenerator user;
        rc = context.FindExistingNode(XN_NODE_TYPE_USER, user);
        if ( rc != XN_STATUS_OK ) {
            throw std::runtime_error(xnGetStatusString( rc ));
        }
        
        // カメラサイズのイメージを作成(8bitのRGB)
        XnMapOutputMode outputMode;
        image.GetMapOutputMode(outputMode);
        camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
                                 IPL_DEPTH_8U, 3);
        if (!camera) {
            throw std::runtime_error("error : cvCreateImage");
        }
        
        // 背景画像を取得
        background = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
                                 IPL_DEPTH_8U, 3);
        if (!background) {
            throw std::runtime_error("error : cvCreateImage");
        }
        
        bool isBackgroundRefresh = true;
        bool isCamouflage = true;
        
        // メインループ
        while (1) {
            // すべてのノードの更新を待つ
            context.WaitAndUpdateAll();
            
            // 画像データの取得
            xn::ImageMetaData imageMD;
            image.GetMetaData(imageMD);
            
            // ユーザーデータの取得
            xn::SceneMetaData sceneMD;
            user.GetUserPixels(0, sceneMD);

            
            // 背景画像の更新
            if (isBackgroundRefresh) {
                isBackgroundRefresh = false;
                memcpy(background->imageData, imageMD.RGB24Data(), background->imageSize);
            }
            
            // カメラ画像の表示
            char* dest = camera->imageData;
            char* back = background->imageData;
            for (int y = 0; y < imageMD.YRes(); ++y) {
                for (int x = 0; x < imageMD.XRes(); ++x) {
                    // 光学迷彩が有効で、ユーザーがいる場合、背景を描画する
                    if (isCamouflage && (sceneMD(x, y) != 0)) {
                        dest[0] = back[0];
                        dest[1] = back[1];
                        dest[2] = back[2];
                    }
                    // ユーザーではないので、カメラ画像を描画する
                    else {
                        // カメラ画像の表示
                        XnRGB24Pixel rgb = imageMD.RGB24Map()(x, y);
                        dest[0] = rgb.nRed;
                        dest[1] = rgb.nGreen;
                        dest[2] = rgb.nBlue;
                    }
                    
                    dest += 3;
                    back += 3;
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
            // 背景の更新
            else if (key == 'r') {
                isBackgroundRefresh = true;
            }
            // 迷彩の入り/切り
            else if (key == 'c') {
                isCamouflage = !isCamouflage;
            }
        }
    }
    catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    ::cvReleaseImage(&background);
    ::cvReleaseImage(&camera);
    
    return 0;
}
