package com.shadowmask.dialog

import android.content.Context
import com.shadowmask.core.R
import com.shadowmask.core.di.ServiceLocator
import com.shadowmask.core.download.DownloadEngine
import com.shadowmask.core.download.Subject
import com.shadowmask.core.model.module.OnlineModule
import com.shadowmask.ui.flash.FlashFragment
import com.shadowmask.view.ShadowMaskDialog
import com.shadowmask.view.Notifications
import kotlinx.parcelize.Parcelize

class OnlineModuleInstallDialog(private val item: OnlineModule) : MarkDownDialog() {

    private val svc get() = ServiceLocator.networkService

    override suspend fun getMarkdownText(): String {
        val str = svc.fetchString(item.changelog)
        return if (str.length > 1000) str.substring(0, 1000) else str
    }

    @Parcelize
    class Module(
        override val module: OnlineModule,
        override val autoLaunch: Boolean,
        override val notifyId: Int = Notifications.nextId()
    ) : Subject.Module() {
        override fun pendingIntent(context: Context) = FlashFragment.installIntent(context, file)
    }

    override fun build(dialog: ShadowMaskDialog) {
        super.build(dialog)
        dialog.apply {

            fun download(install: Boolean) {
                DownloadEngine.startWithActivity(activity, Module(item, install))
            }

            val title = context.getString(R.string.repo_install_title,
                item.name, item.version, item.versionCode)

            setTitle(title)
            setCancelable(true)
            setButton(ShadowMaskDialog.ButtonType.NEGATIVE) {
                text = R.string.download
                onClick { download(false) }
            }
            setButton(ShadowMaskDialog.ButtonType.POSITIVE) {
                text = R.string.install
                onClick { download(true) }
            }
            setButton(ShadowMaskDialog.ButtonType.NEUTRAL) {
                text = android.R.string.cancel
            }
        }
    }

}
