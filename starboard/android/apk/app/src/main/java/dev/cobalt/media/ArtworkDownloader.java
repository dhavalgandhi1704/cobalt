package dev.cobalt.media;

/**
 * Interface to download artwork (Bitmap) from a URL, intended for use in media session metadata.
 */
public interface ArtworkDownloader {
  public void downloadArtwork(String url, ArtworkLoader artworkLoader);
}
