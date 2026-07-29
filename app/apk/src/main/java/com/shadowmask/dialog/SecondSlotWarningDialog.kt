package com.shadowmask.dialog

import com.shadowmask.core.R
import com.shadowmask.events.DialogBuilder
import com.shadowmask.view.ShadowMaskDialog

class SecondSlotWarningDialog : DialogBuilder {

    override fun build(dialog: ShadowMaskDialog) {
        dialog.apply {
            setTitle(android.R.string.dialog_alert_title)
            setMessage(R.string.install_inactive_slot_msg)
            setButton(ShadowMaskDialog.ButtonType.POSITIVE) {
                text = android.R.string.ok
            }
            setCancelable(true)
        }
    }
}
