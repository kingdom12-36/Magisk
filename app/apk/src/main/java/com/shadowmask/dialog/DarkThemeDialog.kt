package com.shadowmask.dialog

import android.app.Activity
import androidx.appcompat.app.AppCompatDelegate
import com.shadowmask.R
import com.shadowmask.arch.UIActivity
import com.shadowmask.core.Config
import com.shadowmask.events.DialogBuilder
import com.shadowmask.view.ShadowMaskDialog
import com.shadowmask.core.R as CoreR

class DarkThemeDialog : DialogBuilder {

    override fun build(dialog: ShadowMaskDialog) {
        val activity = dialog.ownerActivity!!
        dialog.apply {
            setTitle(CoreR.string.settings_dark_mode_title)
            setMessage(CoreR.string.settings_dark_mode_message)
            setButton(ShadowMaskDialog.ButtonType.POSITIVE) {
                text = CoreR.string.settings_dark_mode_light
                icon = R.drawable.ic_day
                onClick { selectTheme(AppCompatDelegate.MODE_NIGHT_NO, activity) }
            }
            setButton(ShadowMaskDialog.ButtonType.NEUTRAL) {
                text = CoreR.string.settings_dark_mode_system
                icon = R.drawable.ic_day_night
                onClick { selectTheme(AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM, activity) }
            }
            setButton(ShadowMaskDialog.ButtonType.NEGATIVE) {
                text = CoreR.string.settings_dark_mode_dark
                icon = R.drawable.ic_night
                onClick { selectTheme(AppCompatDelegate.MODE_NIGHT_YES, activity) }
            }
        }
    }

    private fun selectTheme(mode: Int, activity: Activity) {
        Config.darkTheme = mode
        (activity as UIActivity<*>).delegate.localNightMode = mode
    }
}
