package com.shadowmask.view

import com.shadowmask.R
import com.shadowmask.databinding.DiffItem
import com.shadowmask.databinding.ItemWrapper
import com.shadowmask.databinding.RvItem

class TextItem(override val item: Int) : RvItem(), DiffItem<TextItem>, ItemWrapper<Int> {
    override val layoutRes = R.layout.item_text
}
