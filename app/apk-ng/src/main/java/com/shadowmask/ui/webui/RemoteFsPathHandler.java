package com.shadowmask.ui.webui;

import android.content.Context;
import android.util.Log;
import android.webkit.WebResourceResponse;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import androidx.webkit.WebViewAssetLoader;

import com.topjohnwu.superuser.nio.FileSystemManager;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.GZIPInputStream;

/**
 * PathHandler that serves files from the module's webroot directory
 * via root FileSystemManager, so files under /data/adb/modules/ are accessible.
 */
public final class RemoteFsPathHandler implements WebViewAssetLoader.PathHandler {
    private static final String TAG = "RemoteFsPathHandler";

    public static final String DEFAULT_MIME_TYPE = "text/plain";

    @NonNull
    private final File mDirectory;
    private final FileSystemManager mFs;

    public RemoteFsPathHandler(@NonNull Context context, @NonNull File directory,
                               @NonNull FileSystemManager fs) {
        mDirectory = directory;
        mFs = fs;
    }

    @Override
    @WorkerThread
    @Nullable
    public WebResourceResponse handle(@NonNull String path) {
        try {
            File file = getCanonicalFileIfChild(mDirectory, path);
            if (file != null) {
                InputStream is = openFile(file, mFs);
                String mimeType = guessMimeType(path);
                return new WebResourceResponse(mimeType, null, is);
            } else {
                Log.e(TAG, String.format(
                        "Requested file %s is outside module webroot %s", path, mDirectory));
            }
        } catch (IOException e) {
            Log.e(TAG, "Error opening path: " + path, e);
        }
        return new WebResourceResponse(null, null, null);
    }

    public static String getCanonicalDirPath(@NonNull File file) throws IOException {
        String canonicalPath = file.getCanonicalPath();
        if (!canonicalPath.endsWith("/")) canonicalPath += "/";
        return canonicalPath;
    }

    @Nullable
    public static File getCanonicalFileIfChild(@NonNull File parent, @NonNull String child)
            throws IOException {
        String parentCanonicalPath = getCanonicalDirPath(parent);
        String childCanonicalPath = new File(parent, child).getCanonicalPath();
        if (childCanonicalPath.startsWith(parentCanonicalPath)) {
            return new File(childCanonicalPath);
        }
        return null;
    }

    @NonNull
    private static InputStream handleSvgzStream(@NonNull String path,
                                                @NonNull InputStream stream) throws IOException {
        return path.endsWith(".svgz") ? new GZIPInputStream(stream) : stream;
    }

    public static InputStream openFile(@NonNull File file, @NonNull FileSystemManager fs)
            throws IOException {
        return handleSvgzStream(file.getPath(),
                fs.getFile(file.getAbsolutePath()).newInputStream());
    }

    @NonNull
    public static String guessMimeType(@NonNull String filePath) {
        String mimeType = MimeUtil.getMimeFromFileName(filePath);
        return mimeType == null ? DEFAULT_MIME_TYPE : mimeType;
    }
}
