package com.shadowmask.ui.module

import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.shadowmask.arch.AsyncLoadViewModel
import com.shadowmask.core.Const
import com.shadowmask.core.Info
import com.shadowmask.core.R as CoreR
import com.shadowmask.core.download.Subject
import com.shadowmask.core.model.module.LocalModule
import com.shadowmask.core.model.module.OnlineModule
import com.shadowmask.core.utils.MediaStoreUtils
import com.shadowmask.core.utils.TextHolder
import com.shadowmask.core.utils.asText
import com.shadowmask.ui.flash.FlashUtils
import com.shadowmask.ui.navigation.Route
import com.shadowmask.view.Notifications
import kotlinx.parcelize.IgnoredOnParcel
import kotlinx.parcelize.Parcelize
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.withContext

class ModuleItem(val module: LocalModule) {
    val showNotice: Boolean
    val showAction: Boolean
    val showWebUi: Boolean
    val noticeText: TextHolder

    init {
        val isZygisk = module.isZygisk
        val isRiru = module.isRiru
        val zygiskUnloaded = isZygisk && module.zygiskUnloaded

        showNotice = zygiskUnloaded ||
            (Info.isZygiskEnabled && isRiru) ||
            (!Info.isZygiskEnabled && isZygisk)
        showAction = module.hasAction && !showNotice
        showWebUi = module.hasWebUi && !showNotice
        noticeText =
            when {
                zygiskUnloaded -> CoreR.string.zygisk_module_unloaded.asText()
                isRiru -> CoreR.string.suspend_text_riru.asText(CoreR.string.zygisk.asText())
                else -> CoreR.string.suspend_text_zygisk.asText(CoreR.string.zygisk.asText())
            }
    }

    var isEnabled by mutableStateOf(module.enable)
    var isRemoved by mutableStateOf(module.remove)
    var showUpdate by mutableStateOf(module.updateInfo != null)
    val isUpdated = module.updated
    val updateReady get() = module.outdated && !isRemoved && isEnabled
}

/** Directly downloads a file from [downloadUrl] and flashes (zip) or installs (apk) it. */
@Parcelize
class NetworkInstallSubject(
    val downloadUrl: String,
    val fileName: String,
    override val notifyId: Int = Notifications.nextId()
) : Subject() {
    override val url: String get() = downloadUrl
    override val title: String get() = fileName

    @IgnoredOnParcel
    override val file by lazy { MediaStoreUtils.getFile(fileName).uri }

    override val autoLaunch: Boolean get() = true

    override fun pendingIntent(context: Context): PendingIntent? =
        when {
            fileName.endsWith(".zip", ignoreCase = true) ->
                FlashUtils.installIntent(context, file)
            fileName.endsWith(".apk", ignoreCase = true) -> {
                val intent = Intent(Intent.ACTION_VIEW).apply {
                    setDataAndType(file, "application/vnd.android.package-archive")
                    flags = Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_ACTIVITY_NEW_TASK
                }
                PendingIntent.getActivity(
                    context, notifyId, intent,
                    PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_ONE_SHOT
                )
            }
            else -> null
        }
}

@Parcelize
class OnlineModuleSubject(
    override val module: OnlineModule,
    override val autoLaunch: Boolean,
    override val notifyId: Int = Notifications.nextId()
) : Subject.Module() {
    override fun pendingIntent(context: Context) = FlashUtils.installIntent(context, file)
}

class ModuleViewModel : AsyncLoadViewModel() {

    data class UiState(
        val loading: Boolean = true,
        val modules: List<ModuleItem> = emptyList(),
    )

    private val _uiState = MutableStateFlow(UiState())
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    override suspend fun doLoadWork() {
        _uiState.update { it.copy(loading = true) }
        val moduleLoaded = Info.env.isActive &&
            withContext(Dispatchers.IO) { LocalModule.loaded() }
        if (moduleLoaded) {
            val modules = withContext(Dispatchers.Default) {
                LocalModule.installed().map { ModuleItem(it) }
            }
            _uiState.update { it.copy(loading = false, modules = modules) }
            loadUpdateInfo()
        } else {
            _uiState.update { it.copy(loading = false) }
        }
    }

    private val networkObserver: (Boolean) -> Unit = { startLoading() }

    init {
        Info.isConnected.observeForever(networkObserver)
    }

    override fun onCleared() {
        super.onCleared()
        Info.isConnected.removeObserver(networkObserver)
    }

    private suspend fun loadUpdateInfo() {
        withContext(Dispatchers.IO) {
            _uiState.value.modules.forEach { item ->
                if (item.module.fetch()) {
                    item.showUpdate = item.module.updateInfo != null
                }
            }
        }
    }

    fun confirmLocalInstall(uri: Uri) {
        navigateTo(Route.Flash(Const.Value.FLASH_ZIP, uri.toString()))
    }

    fun runAction(id: String, name: String) {
        navigateTo(Route.Action(id, name))
    }

    fun openWebUi(context: Context, id: String, name: String) {
        val intent = Intent(context, com.shadowmask.ui.webui.WebUIActivity::class.java).apply {
            putExtra("id", id)
            putExtra("name", name)
        }
        context.startActivity(intent)
    }

    fun toggleEnabled(item: ModuleItem) {
        item.isEnabled = !item.isEnabled
        item.module.enable = item.isEnabled
    }

    fun toggleRemove(item: ModuleItem) {
        item.isRemoved = !item.isRemoved
        item.module.remove = item.isRemoved
    }
}
