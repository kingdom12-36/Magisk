package com.shadowmask.ui.home

import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.widget.Toast
import androidx.core.net.toUri
import androidx.databinding.Bindable
import com.shadowmask.BR
import com.shadowmask.R
import com.shadowmask.arch.ActivityExecutor
import com.shadowmask.arch.AsyncLoadViewModel
import com.shadowmask.arch.ContextExecutor
import com.shadowmask.arch.UIActivity
import com.shadowmask.arch.ViewEvent
import com.shadowmask.core.BuildConfig
import com.shadowmask.core.Config
import com.shadowmask.core.Info
import com.shadowmask.core.download.Subject
import com.shadowmask.core.download.Subject.App
import com.shadowmask.core.ktx.await
import com.shadowmask.core.ktx.toast
import com.shadowmask.core.repository.NetworkService
import com.shadowmask.core.utils.asText
import com.shadowmask.databinding.bindExtra
import com.shadowmask.databinding.set
import com.shadowmask.dialog.EnvFixDialog
import com.shadowmask.dialog.ManagerInstallDialog
import com.shadowmask.dialog.UninstallDialog
import com.shadowmask.events.SnackbarEvent
import com.topjohnwu.superuser.Shell
import kotlin.math.roundToInt
import com.shadowmask.core.R as CoreR

class HomeViewModel(
    private val svc: NetworkService
) : AsyncLoadViewModel() {

    enum class State {
        LOADING, INVALID, OUTDATED, UP_TO_DATE
    }

    val shadowmaskTitleBarrierIds =
        intArrayOf(R.id.home_shadow_mask_icon, R.id.home_shadow_mask_title, R.id.home_shadow_mask_button)
    val appTitleBarrierIds =
        intArrayOf(R.id.home_manager_icon, R.id.home_manager_title, R.id.home_manager_button)

    @get:Bindable
    var isNoticeVisible = Config.safetyNotice
        set(value) = set(value, field, { field = it }, BR.noticeVisible)

    val shadowmaskState
        get() = when {
            Info.isRooted && Info.env.isUnsupported -> State.OUTDATED
            !Info.env.isActive -> State.INVALID
            Info.env.versionCode < BuildConfig.APP_VERSION_CODE -> State.OUTDATED
            else -> State.UP_TO_DATE
        }

    @get:Bindable
    var appState = State.LOADING
        set(value) = set(value, field, { field = it }, BR.appState)

    val shadowmaskInstalledVersion
        get() = Info.env.run {
            if (isActive)
                ("$versionString ($versionCode)" + if (isDebug) " (D)" else "").asText()
            else
                CoreR.string.not_available.asText()
        }

    @get:Bindable
    var managerRemoteVersion = CoreR.string.loading.asText()
        set(value) = set(value, field, { field = it }, BR.managerRemoteVersion)

    val managerInstalledVersion
        get() = "${BuildConfig.APP_VERSION_NAME} (${BuildConfig.APP_VERSION_CODE})" +
            if (BuildConfig.DEBUG) " (D)" else ""

    @get:Bindable
    var stateManagerProgress = 0
        set(value) = set(value, field, { field = it }, BR.stateManagerProgress)

    val extraBindings = bindExtra {
        it.put(BR.viewModel, this)
    }

    companion object {
        private var checkedEnv = false
    }

    override suspend fun doLoadWork() {
        appState = State.LOADING
        Info.fetchUpdate(svc)?.apply {
            appState = when {
                BuildConfig.APP_VERSION_CODE < versionCode -> State.OUTDATED
                else -> State.UP_TO_DATE
            }

            val isDebug = Config.updateChannel == Config.Value.DEBUG_CHANNEL
            managerRemoteVersion =
                ("$version (${versionCode})" + if (isDebug) " (D)" else "").asText()
        } ?: run {
            appState = State.INVALID
            managerRemoteVersion = CoreR.string.not_available.asText()
        }
        ensureEnv()
    }

    override fun onNetworkChanged(network: Boolean) = startLoading()

    fun onProgressUpdate(progress: Float, subject: Subject) {
        if (subject is App)
            stateManagerProgress = progress.times(100f).roundToInt()
    }

    fun onLinkPressed(link: String) = object : ViewEvent(), ContextExecutor {
        override fun invoke(context: Context) {
            val intent = Intent(Intent.ACTION_VIEW, link.toUri())
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            try {
                context.startActivity(intent)
            } catch (e: ActivityNotFoundException) {
                context.toast(CoreR.string.open_link_failed_toast, Toast.LENGTH_SHORT)
            }
        }
    }.publish()

    fun onDeletePressed() = UninstallDialog().show()

    fun onManagerPressed() = when (appState) {
        State.LOADING -> SnackbarEvent(CoreR.string.loading).publish()
        State.INVALID -> SnackbarEvent(CoreR.string.no_connection).publish()
        else -> withExternalRW {
            withInstallPermission {
                ManagerInstallDialog().show()
            }
        }
    }

    fun onShadowMaskPressed() = withExternalRW {
        HomeFragmentDirections.actionHomeFragmentToInstallFragment().navigate()
    }

    fun hideNotice() {
        Config.safetyNotice = false
        isNoticeVisible = false
    }

    private suspend fun ensureEnv() {
        if (shadowmaskState == State.INVALID || checkedEnv) return
        val cmd = "env_check ${Info.env.versionString} ${Info.env.versionCode}"
        val code = Shell.cmd(cmd).await().code
        if (code != 0) {
            EnvFixDialog(this, code).show()
        }
        checkedEnv = true
    }

    val showTest = false
    fun onTestPressed() = object : ViewEvent(), ActivityExecutor {
        override fun invoke(activity: UIActivity<*>) {
            /* Entry point to trigger test events within the app */
        }
    }.publish()
}
