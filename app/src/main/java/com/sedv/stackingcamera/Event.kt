package com.sedv.stackingcamera

class Event<T> where T : Function<*> {
    private val handlers = mutableSetOf<T>()

    operator fun plusAssign(handler: T) {
        handlers.add(handler)
    }
    operator fun minusAssign(handler: T) {
        handlers.remove(handler)
    }

    fun invokeAll(invoker: (T) -> Any?) {
        handlers.forEach { invoker(it) }
    }

    fun clear() {
        handlers.clear()
    }
}