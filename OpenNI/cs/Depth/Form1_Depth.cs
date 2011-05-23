using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace Depth
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private xn.Context context;
    private xn.ImageGenerator image;
    private xn.DepthGenerator depth;

    // 描画に必要なオブジェクト
    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("ＭＳ ゴシック", 30);

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

      // デプスジェネレータの作成
      depth = context.FindExistingNode(xn.NodeType.Depth)
                                              as xn.DepthGenerator;
      if (depth == null) {
        throw new Exception(context.GetGlobalErrorState());
      }
    }

    // 描画
    private unsafe void xnDraw()
    {
      // ノードの更新を待ち、データを取得する
      context.WaitAndUpdateAll();
      xn.ImageMetaData imageMD = image.GetMetaData();
      xn.DepthMetaData depthMD = depth.GetMetaData();

      // カメラ画像の作成
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


        // 中心点の距離を表示
        Graphics g = Graphics.FromImage(bitmap);

        int x = (int)image.GetMapOutputMode().nXRes / 2;
        int y = (int)image.GetMapOutputMode().nYRes / 2;
        g.FillEllipse(brush, x - 10, y - 10, 20, 20);

        string message = depthMD[x, y] + "mm";
        g.DrawString(message, font, brush, x, y);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
    }
  }
}
