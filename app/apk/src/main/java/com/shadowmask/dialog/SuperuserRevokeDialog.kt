package com.shadowmask.dialog

import com.shadowmask.core.R
import com.shadowmask.events.DialogBuilder
import com.shadowmask.view.ShadowMaskDialog

class SuperuserRevokeDialog(
    private val appName: String,
    private val onSuccess: () -> Unit
) : DialogBuilder {

    override fun build(dialog: ShadowMaskDialog) {
        dialog.apply {
            setTitle(R.string.su_revoke_title)
            setMessage(R.string.su_revoke_msg, appName)
            setButton(ShadowMaskDialog.ButtonType.POSITIVE) {
                text = android.R.string.ok
                onClick { onSuccess() }
            }
            setButton(ShadowMaskDialog.ButtonType.NEGATIVE) {
                text = android.R.string.cancel
            }
        }
    }
}
