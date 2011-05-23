using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace User
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private xn.Context context;
    private xn.ImageGenerator image;
    private xn.DepthGenerator depth;
    private xn.UserGenerator user;

    // 描画用
    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("Arial", 30);
    private PointF point = new PointF(0, 0);

    private string message = "";

    // ユーザーの色づけ
    float[,] colors = new float[,]
        {
	        {1,1,1},    // ユーザーなし
	        {0,1,1},    {0,0,1},    {0,1,0},
	        {1,1,0},    {1,0,0},    {1,0.5F,0},
	        {0.5F,1,0}, {0,0.5F,1}, {0.5F,0,1},
	        {1,1,0.5F},
        };

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

      // デプスの座標をイメージに合わせる
      depth.GetAlternativeViewPointCap().SetViewPoint(image);

      // ユーザージェネレータの作成
      user = context.FindExistingNode(xn.NodeType.User)
                                              as xn.UserGenerator;
      if (depth == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // ユーザー検出機能をサポートしているか確認
      if (!user.IsCapabilitySupported("User::Skeleton")) {
        throw new Exception("ユーザー検出をサポートしていません");
      }

      // ユーザー認識のコールバックを登録
      user.NewUser += new xn.UserGenerator.NewUserHandler(user_NewUser);
      user.LostUser += new xn.UserGenerator.LostUserHandler(user_LostUser);
    }

    // 新しユーザーの検出
    void user_NewUser(xn.ProductionNode node, uint id)
    {
      message = "ユーザー検出:" + id;
    }

    // ユーザーの消失
    void user_LostUser(xn.ProductionNode node, uint id)
    {
      message = "ユーザー消失:" + id;
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      xn.ImageMetaData imageMD = image.GetMetaData();
      xn.SceneMetaData sceneMD = user.GetUserPixels(0);

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                        System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.GetImageMapPtr().ToPointer();
        ushort* label = (ushort*)sceneMD.SceneMapPtr.ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3, ++label) {
          dst[0] = (byte)(src[2] * colors[*label, 0]);
          dst[1] = (byte)(src[1] * colors[*label, 1]);
          dst[2] = (byte)(src[0] * colors[*label, 2]);
        }

        bitmap.UnlockBits(data);


        // 現在の状態を表示する
        Graphics g = Graphics.FromImage(bitmap);
        g.DrawString(message, font, brush, point);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
    }
  }
}
