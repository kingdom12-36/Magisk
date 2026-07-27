package com.shadowmask.arch

import android.widget.Toast
import androidx.annotation.StringRes
import androidx.lifecycle.ViewModel
import com.shadowmask.core.AppContext
import com.shadowmask.core.ktx.toast
import com.shadowmask.ui.navigation.Route
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow

abstract class BaseViewModel : ViewModel() {

    private val _navEvents = MutableSharedFlow<Route>(extraBufferCapacity = 1)
    val navEvents: SharedFlow<Route> = _navEvents

    fun showSnackbar(@StringRes resId: Int) {
        AppContext.toast(resId, Toast.LENGTH_SHORT)
    }

    fun showSnackbar(msg: String) {
        AppContext.toast(msg, Toast.LENGTH_SHORT)
    }

    fun navigateTo(route: Route) {
        _navEvents.tryEmit(route)
    }
}
