package com.cranklab;

import android.app.Activity;
import android.app.Dialog;
import android.graphics.Color;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.FrameLayout;

/** Provides in-app interactive authentication with itch.io via WebView. */
public final class CrankItchAuth {
    private static volatile Dialog activeDialog = null;
    private static volatile String authStatus = "0"; // 0: idle, 1: authenticating, 2: success, 3: cancelled, 4: error
    private static volatile String authKey = "";
    private static volatile String authMessage = "";

    private CrankItchAuth() {}

    public static synchronized boolean isBusy() {
        return "1".equals(authStatus);
    }

    public static synchronized String[] poll() {
        return new String[] { authStatus, authKey, authMessage };
    }

    public static synchronized void reset() {
        authStatus = "0";
        authKey = "";
        authMessage = "";
    }

    public static void openLogin(final Activity activity) {
        if (activity == null) return;
        activity.runOnUiThread(() -> {
            try {
                if (activeDialog != null && activeDialog.isShowing()) {
                    activeDialog.dismiss();
                }

                authStatus = "1";
                authKey = "";
                authMessage = "Waiting for Itch.io login...";

                final Dialog dialog = new Dialog(activity, android.R.style.Theme_DeviceDefault_NoActionBar_Fullscreen);
                activeDialog = dialog;

                FrameLayout layout = new FrameLayout(activity);
                layout.setBackgroundColor(Color.parseColor("#1f1f1f"));

                final WebView webView = new WebView(activity);
                WebSettings settings = webView.getSettings();
                settings.setJavaScriptEnabled(true);
                settings.setDomStorageEnabled(true);
                settings.setDatabaseEnabled(true);
                settings.setUseWideViewPort(true);
                settings.setLoadWithOverviewMode(true);
                settings.setSupportZoom(true);
                settings.setBuiltInZoomControls(true);
                settings.setDisplayZoomControls(false);

                CookieManager.getInstance().setAcceptCookie(true);
                CookieManager.getInstance().setAcceptThirdPartyCookies(webView, true);

                webView.setWebChromeClient(new WebChromeClient());
                webView.setWebViewClient(new WebViewClient() {
                    private boolean redirectedToKeys = false;

                    @Override
                    public void onPageFinished(WebView view, String url) {
                        super.onPageFinished(view, url);
                        if (url == null) return;

                        String cookies = CookieManager.getInstance().getCookie("https://itch.io");
                        boolean hasSession = (cookies != null && (cookies.contains("session=") || cookies.contains("itchio_token=")));

                        if (hasSession && !redirectedToKeys && (url.contains("/dashboard") || url.equals("https://itch.io/") || url.contains("/my-feed") || url.contains("itch.io/profile"))) {
                            redirectedToKeys = true;
                            authMessage = "Logged in! Fetching API key...";
                            view.loadUrl("https://itch.io/user/settings/api-keys");
                            return;
                        }

                        if (url.contains("/user/settings/api-keys")) {
                            view.evaluateJavascript(
                                "(function() { " +
                                "   var inputs = document.querySelectorAll('input.api_key_input, input[readonly]'); " +
                                "   for (var i = 0; i < inputs.length; i++) { " +
                                "       if (inputs[i].value && inputs[i].value.length > 20) return inputs[i].value; " +
                                "   } " +
                                "   var keyEls = document.querySelectorAll('.api_key, code'); " +
                                "   for (var i = 0; i < keyEls.length; i++) { " +
                                "       var t = keyEls[i].innerText.trim(); " +
                                "       if (t.length > 20) return t; " +
                                "   } " +
                                "   return ''; " +
                                "})()",
                                value -> {
                                    if (value != null && value.length() > 22 && !value.equals("\"\"")) {
                                        String cleanKey = value.replace("\"", "").trim();
                                        authStatus = "2";
                                        authKey = cleanKey;
                                        authMessage = "Success! Connected to Itch.io.";
                                        if (dialog.isShowing()) dialog.dismiss();
                                    }
                                }
                            );
                        }
                    }
                });

                dialog.setOnCancelListener(d -> {
                    authStatus = "3";
                    authMessage = "Login cancelled by user.";
                    activeDialog = null;
                });

                layout.addView(webView, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

                dialog.setContentView(layout);
                dialog.show();

                webView.loadUrl("https://itch.io/login");
            } catch (Exception e) {
                authStatus = "4";
                authMessage = "Failed to launch web login: " + e.getMessage();
            }
        });
    }

    public static void close() {
        if (activeDialog != null) {
            activeDialog.dismiss();
            activeDialog = null;
        }
    }
}
