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
        
        // イメージジェネレータの作成
        xn::ImageGenerator image;
        rc = context.FindExistingNode(XN_NODE_TYPE_IMAGE, image);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        xn::DepthGenerator depth;
        rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depth);
        if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
        }
        
        depth.GetAlternativeViewPointCap().SetViewPoint(image);
        
        xn::SceneAnalyzer scene;
        rc = context.FindExistingNode(XN_NODE_TYPE_SCENE, scene);
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
        
        // ラベルによる色分け
        const XnRGB24Pixel colors[] = {
            { 0, 0, 0 },
            { 255, 0, 0 },
            { 0, 255, 0 },
            { 0, 0, 255 },
            { 255, 255, 0 },
            { 255, 0, 255 },
            { 0, 255, 255 },
            { 255, 255, 255 },
        };
        
        const int colorsCount = sizeof(colors) / sizeof(colors[0]);
        
        // メインループ
        while (1) {
            // 更新を待ち、画像データを取得する
            context.WaitAndUpdateAll();
            
            xn::ImageMetaData imageMD;
            image.GetMetaData(imageMD);
            
            xn::SceneMetaData sceneMD;
            scene.GetMetaData(sceneMD);

            // ラベルの色で上書き
            const xn::LabelMap& labels = sceneMD.LabelMap();
            xn::RGB24Map& rgb = imageMD.WritableRGB24Map();
            
            for (XnUInt y = 0; y < imageMD.YRes(); ++y) {
                for (XnUInt x = 0; x < imageMD.XRes(); ++x) {
                    XnLabel label = labels(x, y);
                    if ( label != 0 ) {
                        if (label < colorsCount) {
                            rgb(x, y) = colors[label];
                        }
                        else {
                            std::cout << label << ", ";
                        }
                    }
                }
            }
            
            // カメラ画像の表示
            //  Kinectからの入力がBGRであるため、RGBに変換して表示する
            // イメージをデプスマップで上書きする
            memcpy(camera->imageData, imageMD.RGB24Data(), camera->imageSize);
            ::cvCvtColor(camera, camera, CV_RGB2BGR);
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
