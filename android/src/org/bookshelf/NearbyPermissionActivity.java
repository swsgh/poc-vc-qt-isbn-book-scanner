package org.bookshelf;

import android.content.pm.PackageManager;

import org.qtproject.qt.android.bindings.QtActivity;

public class NearbyPermissionActivity extends QtActivity {
    private int nearbyPermissionRequestCode = -1;

    public void requestNearbyNetworkPermission(String permission, int requestCode) {
        nearbyPermissionRequestCode = requestCode;
        requestPermissions(new String[] { permission }, requestCode);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != nearbyPermissionRequestCode) {
            return;
        }

        nearbyPermissionRequestCode = -1;
        boolean granted = grantResults.length > 0
                && grantResults[0] == PackageManager.PERMISSION_GRANTED;
        nativeNearbyPermissionResult(requestCode, granted);
    }

    private static native void nativeNearbyPermissionResult(int requestCode, boolean granted);
}