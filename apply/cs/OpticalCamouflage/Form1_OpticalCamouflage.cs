using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using OpenNI;

namespace OpticalCamouflage
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

    private Bitmap background;

    private bool isBackgroundRefresh = true;
    private bool isCamouflage = true;

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      context = new Context(CONFIG_XML_PATH);

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

      // 背景イメージを作成
      MapOutputMode mapMode = image.MapOutputMode;
      background = new Bitmap((int)mapMode.XRes, (int)mapMode.YRes,
                  System.Drawing.Imaging.PixelFormat.Format24bppRgb);

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
        Rectangle rect = new Rectangle(0, 0, this.bitmap.Width, this.bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                            System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 背景のビットマップデータを作成
        BitmapData back = background.LockBits(rect, ImageLockMode.ReadWrite,
                            System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* bk = (byte*)back.Scan0.ToPointer();
        byte* src = (byte*)image.ImageMapPtr.ToPointer();
        ushort* label = (ushort*)sceneMD.LabelMapPtr.ToPointer();

        // 背景の更新
        if (isBackgroundRefresh) {
          isBackgroundRefresh = false;

          for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, bk += 3) {
            bk[0] = src[2];
            bk[1] = src[1];
            bk[2] = src[0];
          }

          bk = (byte*)back.Scan0.ToPointer();
          src = (byte*)image.ImageMapPtr.ToPointer();
        }

        // 画面用の描画
        for (int i = 0; i < imageMD.DataSize;
                          i += 3, src += 3, dst += 3, bk += 3, ++label) {
          // 光学迷彩が有効で、ユーザーがいる場合、背景を描画する
          if (isCamouflage && (*label != 0)) {
            dst[0] = bk[0];
            dst[1] = bk[1];
            dst[2] = bk[2];
          }
          // ユーザーではないので、カメラ画像を描画する
          else {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
          }
        }

        bitmap.UnlockBits(data);
        background.UnlockBits(back);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
      // 背景を更新する
      if (key == Keys.R) {
        isBackgroundRefresh = true;
      }
      // 迷彩の入り/切り
      else if (key == Keys.C) {
        isCamouflage = !isCamouflage;
      }
    }
  }
}
