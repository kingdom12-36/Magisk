package com.shadowmask.utils

import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalResources
import com.shadowmask.core.utils.TextHolder

@Composable
fun textHolder(holder: TextHolder) = holder.getText(LocalResources.current)
