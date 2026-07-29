package com.shadowmask.ui.log

import android.system.Os
import androidx.lifecycle.viewModelScope
import com.shadowmask.arch.AsyncLoadViewModel
import com.shadowmask.core.BuildConfig
import com.shadowmask.core.Info
import com.shadowmask.core.R
import com.shadowmask.core.ktx.timeFormatStandard
import com.shadowmask.core.ktx.toTime
import com.shadowmask.core.model.su.SuLog
import com.shadowmask.core.repository.LogRepository
import com.shadowmask.core.su.SuEvents
import com.shadowmask.core.utils.MediaStoreUtils
import com.shadowmask.core.utils.MediaStoreUtils.outputStream
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.FileInputStream

class LogViewModel(
    private val repo: LogRepository
) : AsyncLoadViewModel() {

    init {
        @OptIn(kotlinx.coroutines.FlowPreview::class)
        viewModelScope.launch {
            SuEvents.logUpdated.debounce(500).collect { reload() }
        }
    }

    data class UiState(
        val loading: Boolean = true,
        val shadowmaskLog: String = "",
        val shadowmaskLogEntries: List<ShadowMaskLogEntry> = emptyList(),
        val suLogs: List<SuLog> = emptyList(),
    )

    private val _uiState = MutableStateFlow(UiState())
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    private var shadowmaskLogRaw = ""

    override suspend fun doLoadWork() {
        _uiState.update { it.copy(loading = true) }
        withContext(Dispatchers.Default) {
            shadowmaskLogRaw = repo.fetchShadowMaskLogs()
            val suLogs = repo.fetchSuLogs()
            val entries = ShadowMaskLogParser.parse(shadowmaskLogRaw)
            _uiState.update { it.copy(
                loading = false,
                shadowmaskLog = shadowmaskLogRaw,
                shadowmaskLogEntries = entries,
                suLogs = suLogs,
            ) }
        }
    }

    fun saveShadowMaskLog() {
        viewModelScope.launch(Dispatchers.IO) {
            val filename = "shadowmask_log_%s.log".format(
                System.currentTimeMillis().toTime(timeFormatStandard))
            val logFile = MediaStoreUtils.getFile(filename)
            logFile.uri.outputStream().bufferedWriter().use { file ->
                file.write("---Detected Device Info---\n\n")
                file.write("isAB=${Info.isAB}\n")
                file.write("isSAR=${Info.isSAR}\n")
                file.write("ramdisk=${Info.ramdisk}\n")
                val uname = Os.uname()
                file.write("kernel=${uname.sysname} ${uname.machine} ${uname.release} ${uname.version}\n")

                file.write("\n\n---System Properties---\n\n")
                ProcessBuilder("getprop").start()
                    .inputStream.reader().use { it.copyTo(file) }

                file.write("\n\n---Environment Variables---\n\n")
                System.getenv().forEach { (key, value) -> file.write("${key}=${value}\n") }

                file.write("\n\n---System MountInfo---\n\n")
                FileInputStream("/proc/self/mountinfo").reader().use { it.copyTo(file) }

                file.write("\n---ShadowMask Logs---\n")
                file.write("${Info.env.versionString} (${Info.env.versionCode})\n\n")
                if (Info.env.isActive) file.write(shadowmaskLogRaw)

                file.write("\n---Manager Logs---\n")
                file.write("${BuildConfig.APP_VERSION_NAME} (${BuildConfig.APP_VERSION_CODE})\n\n")
                ProcessBuilder("logcat", "-d").start()
                    .inputStream.reader().use { it.copyTo(file) }
            }
            showSnackbar(logFile.toString())
        }
    }

    fun clearShadowMaskLog() = repo.clearShadowMaskLogs {
        showSnackbar(R.string.logs_cleared)
        startLoading()
    }

    fun clearLog() = viewModelScope.launch {
        repo.clearLogs()
        showSnackbar(R.string.logs_cleared)
        startLoading()
    }
}
