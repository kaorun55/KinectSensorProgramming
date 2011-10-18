using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using OpenNI;

namespace DrawUser
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private Context context;
    private ImageGenerator image;
    private DepthGenerator depth;
    private UserGenerator user;

    private bool isBackground = false;

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      ScriptNode scriptNode;
      context = Context.CreateFromXmlFile( CONFIG_XML_PATH, out scriptNode );

      // イメージジェネレータの作成
      image = context.FindExistingNode(NodeType.Image) as ImageGenerator;
      if (image == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // デプスジェネレータの作成
      depth = context.FindExistingNode(NodeType.Depth) as DepthGenerator;
      if (depth == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // デプスの座標をイメージに合わせる
      depth.AlternativeViewpointCapability.SetViewpoint(image);

      // ユーザージェネレータの作成
      user = context.FindExistingNode(NodeType.User) as UserGenerator;
      if (depth == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // ユーザー検出機能をサポートしているか確認
      if (!user.IsCapabilitySupported("User::Skeleton")) {
        throw new Exception("ユーザー検出をサポートしていません");
      }
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      ImageMetaData imageMD = image.GetMetaData();
      SceneMetaData sceneMD = user.GetUserPixels(0);

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                            System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.ImageMapPtr.ToPointer();
        ushort* label = (ushort*)sceneMD.LabelMapPtr.ToPointer();

        // 画面用の描画
        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3, ++label) {
          // 背景描画が有効か、ユーザーがいる場合、カメラ画像を描画する
          if (isBackground || (*label != 0)) {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
          }
          // その他の場合は描画しない
          else {
            dst[0] = 255;
            dst[1] = 255;
            dst[2] = 255;
          }
        }

        bitmap.UnlockBits(data);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
      // 背景表示の入り/切り
      if (key == Keys.B) {
          isBackground = !isBackground;
      }
    }
  }
}
