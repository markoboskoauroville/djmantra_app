package com.djmantra.app;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.UriPermission;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.util.Log;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Drives through the Storage Access Framework, for the native side
 * (src/sources/androidstorage.cpp). USB sticks are often mounted where apps
 * cannot read them by path; the user grants a "tree" on the drive once, and
 * the app lists and reads it through the ContentResolver.
 *
 * Strings are tab separated so the native side needs no Java objects.
 */
public final class StorageBridge {
    private static final String TAG = "DJMantraStorage";
    private static final String EXTERNAL_STORAGE = "com.android.externalstorage.documents";

    private StorageBridge() {}

    /** Removable drives: "id \t label \t mounted(0/1) \t granted(0/1)". */
    public static String[] volumes(Context context) {
        StorageManager storage = context.getSystemService(StorageManager.class);
        Set<String> granted = grantedVolumeIds(context);
        List<String> out = new ArrayList<>();
        for (StorageVolume volume : storage.getStorageVolumes()) {
            String id = volume.getUuid();
            if (volume.isPrimary() || id == null) {
                continue;
            }
            String state = volume.getState();
            boolean mounted = Environment.MEDIA_MOUNTED.equals(state)
                    || Environment.MEDIA_MOUNTED_READ_ONLY.equals(state);
            String label = volume.getDescription(context);
            out.add(id + "\t" + clean(label) + "\t" + (mounted ? 1 : 0) + "\t"
                    + (granted.contains(id) ? 1 : 0));
        }
        return out.toArray(new String[0]);
    }

    /** Intent for the system folder picker, opened on the drive if given. */
    public static Intent treePickerIntent(Context context, String volumeId) {
        Intent intent = null;
        if (volumeId != null && !volumeId.isEmpty()) {
            StorageManager storage = context.getSystemService(StorageManager.class);
            for (StorageVolume volume : storage.getStorageVolumes()) {
                if (volumeId.equals(volume.getUuid())) {
                    intent = volume.createOpenDocumentTreeIntent();
                    break;
                }
            }
        }
        if (intent == null) {
            intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        }
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        return intent;
    }

    /**
     * Keeps the access the user granted in the picker across restarts.
     * Returns "volume id \t path on the drive" ("primary" for internal
     * storage), or null.
     */
    public static String persistTree(Context context, Intent data) {
        if (data == null || data.getData() == null) {
            return null;
        }
        Uri tree = data.getData();
        try {
            context.getContentResolver().takePersistableUriPermission(
                    tree, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } catch (SecurityException e) {
            Log.w(TAG, "Cannot keep access to " + tree, e);
            return null;
        }
        String docId = DocumentsContract.getTreeDocumentId(tree);
        int colon = docId.indexOf(':');
        if (colon < 0) {
            return null;
        }
        Log.i(TAG, "Granted " + tree);
        return docId.substring(0, colon) + "\t" + docId.substring(colon + 1);
    }

    /**
     * Every file below the granted trees of a drive:
     * "path on the drive \t size \t last modified (ms)". Null if the drive
     * could not be listed completely.
     */
    public static String[] list(Context context, String volumeId) {
        ContentResolver resolver = context.getContentResolver();
        Set<String> seen = new HashSet<>();
        List<String> out = new ArrayList<>();
        String[] columns = {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_MIME_TYPE,
            DocumentsContract.Document.COLUMN_SIZE,
            DocumentsContract.Document.COLUMN_LAST_MODIFIED,
        };
        try {
            for (Uri tree : grantedTrees(context, volumeId)) {
                ArrayDeque<String> pending = new ArrayDeque<>();
                pending.add(DocumentsContract.getTreeDocumentId(tree));
                while (!pending.isEmpty()) {
                    String parent = pending.poll();
                    Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, parent);
                    try (Cursor cursor = resolver.query(children, columns, null, null, null)) {
                        if (cursor == null) {
                            return null;
                        }
                        while (cursor.moveToNext()) {
                            String docId = cursor.getString(0);
                            if (DocumentsContract.Document.MIME_TYPE_DIR.equals(cursor.getString(1))) {
                                pending.add(docId);
                            } else if (seen.add(docId)) {
                                int colon = docId.indexOf(':');
                                out.add(docId.substring(colon + 1) + "\t" + cursor.getLong(2)
                                        + "\t" + cursor.getLong(3));
                            }
                        }
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "Cannot list drive " + volumeId, e);
            return null;
        }
        return out.toArray(new String[0]);
    }

    /** A file descriptor for reading (owned by the caller), or -1. */
    public static int openForReading(Context context, String volumeId, String path) {
        String docId = volumeId + ":" + path;
        for (Uri tree : grantedTrees(context, volumeId)) {
            String treeDoc = DocumentsContract.getTreeDocumentId(tree);
            if (!docId.equals(treeDoc) && !docId.startsWith(treeDoc.endsWith(":") ? treeDoc : treeDoc + "/")) {
                continue;
            }
            Uri document = DocumentsContract.buildDocumentUriUsingTree(tree, docId);
            try {
                ParcelFileDescriptor descriptor =
                        context.getContentResolver().openFileDescriptor(document, "r");
                if (descriptor != null) {
                    return descriptor.detachFd();
                }
            } catch (Exception e) {
                Log.w(TAG, "Cannot open " + document + ": " + e);
            }
        }
        return -1;
    }

    private static List<Uri> grantedTrees(Context context, String volumeId) {
        List<Uri> trees = new ArrayList<>();
        for (UriPermission permission : context.getContentResolver().getPersistedUriPermissions()) {
            Uri uri = permission.getUri();
            if (!permission.isReadPermission() || !EXTERNAL_STORAGE.equals(uri.getAuthority())) {
                continue;
            }
            String docId;
            try {
                docId = DocumentsContract.getTreeDocumentId(uri);
            } catch (IllegalArgumentException e) {
                continue;
            }
            if (volumeId == null || docId.startsWith(volumeId + ":")) {
                trees.add(uri);
            }
        }
        return trees;
    }

    private static Set<String> grantedVolumeIds(Context context) {
        Set<String> ids = new HashSet<>();
        for (Uri tree : grantedTrees(context, null)) {
            String docId = DocumentsContract.getTreeDocumentId(tree);
            int colon = docId.indexOf(':');
            if (colon > 0) {
                ids.add(docId.substring(0, colon));
            }
        }
        return ids;
    }

    /**
     * FIND SONG (Marko 14.10.2026): every audio file on the phone (all volumes Android has
     * indexed: internal storage, SD card, USB stick) whose title, artist, album or file name
     * contains every word of the query. "path\ttitle\tartist\tduration ms" each, at most 300.
     */
    public static String[] findSongs(Context context, String query) {
        List<String> found = new ArrayList<>();
        String[] words = query.trim().toLowerCase().split("\\s+");
        StringBuilder where = new StringBuilder();
        List<String> args = new ArrayList<>();
        for (String word : words) {
            if (word.isEmpty()) {
                continue;
            }
            if (where.length() > 0) {
                where.append(" AND ");
            }
            where.append("(" + MediaStore.Audio.Media.TITLE + " LIKE ? OR "
                    + MediaStore.Audio.Media.ARTIST + " LIKE ? OR "
                    + MediaStore.Audio.Media.ALBUM + " LIKE ? OR "
                    + MediaStore.Audio.Media.DISPLAY_NAME + " LIKE ?)");
            for (int i = 0; i < 4; ++i) {
                args.add("%" + word + "%");
            }
        }
        if (where.length() == 0) {
            return new String[0];
        }
        String[] columns = {MediaStore.Audio.Media.DATA, MediaStore.Audio.Media.TITLE,
                MediaStore.Audio.Media.ARTIST, MediaStore.Audio.Media.DURATION};
        try (Cursor cursor = context.getContentResolver().query(
                MediaStore.Audio.Media.getContentUri(MediaStore.VOLUME_EXTERNAL), columns,
                where.toString(), args.toArray(new String[0]),
                MediaStore.Audio.Media.TITLE + " COLLATE NOCASE ASC")) {
            while (cursor != null && cursor.moveToNext() && found.size() < 300) {
                String path = cursor.getString(0);
                if (path == null) {
                    continue;
                }
                found.add(clean(path) + "\t" + clean(cursor.getString(1)) + "\t"
                        + clean(cursor.getString(2)) + "\t" + cursor.getLong(3));
            }
        } catch (Exception e) {
            Log.w("DJMantraStorage", "findSongs: " + e);
        }
        Log.i("DJMantraStorage", "findSongs '" + query + "': " + found.size());
        return found.toArray(new String[0]);
    }

    private static String clean(String text) {
        return text == null ? "" : text.replace('\t', ' ').replace('\n', ' ');
    }
}
