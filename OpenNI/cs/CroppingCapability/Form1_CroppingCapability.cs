using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using OpenNI;

namespace CroppingCapability
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private Context context;
    private ImageGenerator image;
    private Cropping cropping;

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      context = new Context(CONFIG_XML_PATH);

      // イメージジェネレータの作成
      image = context.FindExistingNode(NodeType.Image)
                                              as ImageGenerator;
      if (image == null) {
        throw new Exception(context.GlobalErrorState);
        throw new Exception("イメージジェネレータの作成に失敗");
      }

      cropping.Enabled= false;
      cropping.XOffset = 0;
      cropping.YOffset = 0;
      cropping.XSize = 500;
      cropping.YSize = 450;
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      ImageMetaData imageMD = image.GetMetaData();

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                               System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        for (int y = 0; y < imageMD.YRes; ++y) {
          byte* dst = (byte*)data.Scan0.ToPointer();
          dst += data.Width * y * 3;
          for (int x = 0; x < imageMD.XRes; ++x, dst += 3) {
            byte* src = (byte*)image.ImageMapPtr.ToPointer();
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
        cropping.Enabled = !cropping.Enabled;
        image.CroppingCapability.Cropping = cropping;
      }
    }
  }
}
