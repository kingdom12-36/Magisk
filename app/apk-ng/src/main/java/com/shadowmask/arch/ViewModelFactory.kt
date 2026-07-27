package com.shadowmask.arch

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.shadowmask.core.di.ServiceLocator
import com.shadowmask.ui.home.HomeViewModel
import com.shadowmask.ui.install.InstallViewModel
import com.shadowmask.ui.log.LogViewModel
import com.shadowmask.ui.superuser.SuperuserViewModel
import com.shadowmask.ui.surequest.SuRequestViewModel

object VMFactory : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return when (modelClass) {
            HomeViewModel::class.java -> HomeViewModel(ServiceLocator.networkService)
            LogViewModel::class.java -> LogViewModel(ServiceLocator.logRepo)
            SuperuserViewModel::class.java -> SuperuserViewModel(ServiceLocator.policyDB)
            InstallViewModel::class.java ->
                InstallViewModel(ServiceLocator.networkService)
            SuRequestViewModel::class.java ->
                SuRequestViewModel(ServiceLocator.policyDB, ServiceLocator.timeoutPrefs)
            else -> modelClass.newInstance()
        } as T
    }
}
