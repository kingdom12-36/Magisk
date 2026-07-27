package com.shadowmask.net;

public interface ResponseListener<T> {
    void onResponse(T response);
}
