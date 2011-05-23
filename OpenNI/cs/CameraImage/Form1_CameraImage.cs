using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace CameraImage
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private xn.Context context;
    private xn.ImageGenerator image;

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化 ... (1)
      context = new xn.Context(CONFIG_XML_PATH);

      // イメージジェネレータの作成 ... (2)
      image = context.FindExistingNode(xn.NodeType.Image) as xn.ImageGenerator;
      if (image == null) {
        throw new Exception(context.GetGlobalErrorState());
      }
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する ... (4)
      context.WaitOneUpdateAll(image);
      xn.ImageMetaData imageMD = image.GetMetaData();

      // カメラ画像の表示 ... (5)
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                                                PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.GetImageMapPtr().ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3) {
          dst[0] = src[2];
          dst[1] = src[1];
          dst[2] = src[0];
        }

        bitmap.UnlockBits(data);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
    }
  }
}
