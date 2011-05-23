using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace CroppingCapability
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private xn.Context context;
    private xn.ImageGenerator image;
    private xn.Cropping cropping;

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      context = new xn.Context(CONFIG_XML_PATH);

      // イメージジェネレータの作成
      image = context.FindExistingNode(xn.NodeType.Image)
                                              as xn.ImageGenerator;
      if (image == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      cropping.bEnabled = false;
      cropping.nXOffset = 0;
      cropping.nYOffset = 0;
      cropping.nXSize = 500;
      cropping.nYSize = 450;
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      xn.ImageMetaData imageMD = image.GetMetaData();

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                                                PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        for (int y = 0; y < imageMD.YRes; ++y) {
          byte* dst = (byte*)data.Scan0.ToPointer();
          dst += data.Width * y * 3;
          for (int x = 0; x < imageMD.XRes; ++x, dst += 3) {
            byte* src = (byte*)image.GetImageMapPtr().ToPointer();
            src += (imageMD.XRes * y * 3) + (x * 3);

            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
          }
        }

        bitmap.UnlockBits(data);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
      if (key == Keys.C) {
        cropping.bEnabled = !cropping.bEnabled;
        image.GetCroppingCap().SetCropping(ref cropping);
      }
    }
  }
}
