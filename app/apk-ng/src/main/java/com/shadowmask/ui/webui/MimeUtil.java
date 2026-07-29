package com.shadowmask.ui.webui;

import java.net.URLConnection;

class MimeUtil {

    public static String getMimeFromFileName(String fileName) {
        if (fileName == null) return null;

        String mimeType = URLConnection.guessContentTypeFromName(fileName);
        if (mimeType != null) return mimeType;

        return guessHardcodedMime(fileName);
    }

    private static String guessHardcodedMime(String fileName) {
        int finalFullStop = fileName.lastIndexOf('.');
        if (finalFullStop == -1) return null;

        final String extension = fileName.substring(finalFullStop + 1).toLowerCase();

        return switch (extension) {
            case "webm" -> "video/webm";
            case "mpeg", "mpg" -> "video/mpeg";
            case "mp3" -> "audio/mpeg";
            case "wasm" -> "application/wasm";
            case "xhtml", "xht", "xhtm" -> "application/xhtml+xml";
            case "flac" -> "audio/flac";
            case "ogg", "oga", "opus" -> "audio/ogg";
            case "wav" -> "audio/wav";
            case "m4a" -> "audio/x-m4a";
            case "gif" -> "image/gif";
            case "jpeg", "jpg", "jfif", "pjpeg", "pjp" -> "image/jpeg";
            case "png" -> "image/png";
            case "apng" -> "image/apng";
            case "svg", "svgz" -> "image/svg+xml";
            case "webp" -> "image/webp";
            case "mht", "mhtml" -> "multipart/related";
            case "css" -> "text/css";
            case "html", "htm", "shtml", "shtm", "ehtml" -> "text/html";
            case "js", "mjs" -> "application/javascript";
            case "xml" -> "text/xml";
            case "mp4", "m4v" -> "video/mp4";
            case "ogv", "ogm" -> "video/ogg";
            case "ico" -> "image/x-icon";
            case "woff" -> "application/font-woff";
            case "gz", "tgz" -> "application/gzip";
            case "json" -> "application/json";
            case "pdf" -> "application/pdf";
            case "zip" -> "application/zip";
            case "bmp" -> "image/bmp";
            case "tiff", "tif" -> "image/tiff";
            default -> null;
        };
    }
}
