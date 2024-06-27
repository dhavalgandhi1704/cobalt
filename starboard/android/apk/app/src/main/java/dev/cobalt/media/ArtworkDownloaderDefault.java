package dev.cobalt.media;

import static dev.cobalt.media.Log.TAG;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Pair;
import dev.cobalt.util.Log;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

/**
 * Default implementation of ArtworkDownloader using java.net.HttpURLConnection to fetch artwork.
 */
public class ArtworkDownloaderDefault implements ArtworkDownloader {

  public ArtworkDownloaderDefault() {}

  public void downloadArtwork(String url, ArtworkLoader artworkLoader) {
    Bitmap bitmap = null;
    HttpURLConnection conn = null;
    InputStream is = null;
    try {
      conn = (HttpURLConnection) new URL(url).openConnection();
      is = conn.getInputStream();
      bitmap = BitmapFactory.decodeStream(is);
    } catch (IOException e) {
      Log.e(TAG, "Could not download artwork", e);
    } finally {
      try {
        if (conn != null) {
          conn.disconnect();
          conn = null;
        }
        if (is != null) {
          is.close();
          is = null;
        }
      } catch (Exception e) {
        Log.e(TAG, "Error closing connection for artwork", e);
      }
    }

    bitmap = artworkLoader.cropTo16x9(bitmap);
    artworkLoader.onDownloadFinished(Pair.create(url, bitmap));
  }
}
