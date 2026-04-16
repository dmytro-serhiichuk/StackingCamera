package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.graphics.SurfaceTexture
import android.opengl.GLES11Ext
import android.opengl.GLES30
import android.opengl.GLSurfaceView
import android.util.Printer
import android.view.Surface
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class GLPreviewSurfaceRenderer(
    private val context: Context,
    private val initWidth: Int,
    private val initHeight: Int,
    private val onSurfaceReady: (Surface) -> Unit,
) : GLSurfaceView.Renderer {

    private var program = 0
    private var vaoId = 0
    private var vboVertices = 0
    private var vboTexCoords = 0
    private var uTexMatrixLocation = -1

    private var cameraTexture: SurfaceTexture? = null
    private var cameraTextureId = 0
    private var cameraSurface: Surface? = null

    // Uniforms
    private var textureSizeHandle = 0
    private var textureHandle = 0
    private var focusModeHandle = 0
    private var zebraPatternModeHandle = 0

    private val vertices = floatArrayOf(
        -1.0f, -1.0f, 0.0f,  // Bottom left
        1.0f, -1.0f, 0.0f,  // Bottom right
        -1.0f,  1.0f, 0.0f,  // Top left
        1.0f,  1.0f, 0.0f   // Top right
    )

    private val texMatrix = FloatArray(16)
    private val textureCoords = floatArrayOf(
        0f, 0f,
        1f, 0f,
        0f, 1f,
        1f, 1f,
    )

    private var focusModeEnabled = false
    private var zebraPatternEnabled = false
    private var surfaceWidth = 0
    private var surfaceHeight = 0


    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        GLES30.glClearColor(0f, 0f, 0f, 1f)
        setupShaders()

        uTexMatrixLocation = GLES30.glGetUniformLocation(program, "uTexMatrix")

        setupVAO()
        setupCameraTexture()
    }

    private fun setupShaders() {
        val vertexShader = compileShader(GLES30.GL_VERTEX_SHADER, loadShader("vertex_shader.glsl"))
        val fragmentShader = compileShader(GLES30.GL_FRAGMENT_SHADER, loadShader("fragment_shader.glsl"))

        program = GLES30.glCreateProgram().also {
            GLES30.glAttachShader(it, vertexShader)
            GLES30.glAttachShader(it, fragmentShader)
            GLES30.glLinkProgram(it)
        }

        textureSizeHandle = GLES30.glGetUniformLocation(program, "u_textureSize")
        textureHandle = GLES30.glGetUniformLocation(program, "u_texture")
        focusModeHandle = GLES30.glGetUniformLocation(program, "u_focusMode")
        zebraPatternModeHandle = GLES30.glGetUniformLocation(program, "u_zebraPattern")
    }

    private fun setupVAO() {
        val vaoIds = IntArray(1)
        val vboIds = IntArray(2)

        GLES30.glGenVertexArrays(1, vaoIds, 0)
        GLES30.glGenBuffers(2, vboIds, 0)

        vaoId = vaoIds[0]
        vboVertices = vboIds[0]
        vboTexCoords = vboIds[1]

        GLES30.glBindVertexArray(vaoId)

        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, vboVertices)
        val vb = createFloatBuffer(vertices)
        GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, vertices.size * 4, vb, GLES30.GL_STATIC_DRAW)
        val posHandle = GLES30.glGetAttribLocation(program, "a_position")
        GLES30.glEnableVertexAttribArray(posHandle)
        GLES30.glVertexAttribPointer(posHandle, 3, GLES30.GL_FLOAT, false, 0, 0)

        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, vboTexCoords)
        val tb = createFloatBuffer(textureCoords)
        GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, textureCoords.size * 4, tb, GLES30.GL_STATIC_DRAW)
        val texHandle = GLES30.glGetAttribLocation(program, "a_texCoord")
        GLES30.glEnableVertexAttribArray(texHandle)
        GLES30.glVertexAttribPointer(texHandle, 2, GLES30.GL_FLOAT, false, 0, 0)

        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, 0)
        GLES30.glBindVertexArray(0)
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        surfaceWidth = width
        surfaceHeight = height
        GLES30.glViewport(0, 0, width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT)
        cameraTexture?.apply {
            updateTexImage()
            getTransformMatrix(texMatrix)
        }


        GLES30.glUseProgram(program)

        GLES30.glUniformMatrix4fv(uTexMatrixLocation, 1, false, texMatrix, 0)

        GLES30.glUniform2f(textureSizeHandle, surfaceWidth.toFloat(), surfaceHeight.toFloat())
        GLES30.glUniform1f(focusModeHandle, if (focusModeEnabled) 1f else 0f)
        GLES30.glUniform1f(zebraPatternModeHandle, if (zebraPatternEnabled) 1f else 0f)

        GLES30.glActiveTexture(GLES30.GL_TEXTURE0)
        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, cameraTextureId)
        GLES30.glUniform1i(textureHandle, 0)

        GLES30.glBindVertexArray(vaoId)
        GLES30.glDrawArrays(GLES30.GL_TRIANGLE_STRIP, 0, 4)
        GLES30.glBindVertexArray(0)
    }

    private fun setupCameraTexture() {
        val textures = IntArray(1)
        GLES30.glGenTextures(1, textures, 0)
        cameraTextureId = textures[0]

        GLES30.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, cameraTextureId)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE)
        GLES30.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE)

        cameraTexture = SurfaceTexture(cameraTextureId)
        cameraTexture!!.setDefaultBufferSize(initWidth, initHeight)
        cameraSurface = Surface(cameraTexture)
        onSurfaceReady(cameraSurface!!)
    }

    private fun loadShader(name: String) =
        context.assets.open("shaders/$name").bufferedReader().use { it.readText() }

    private fun compileShader(type: Int, code: String) =
        GLES30.glCreateShader(type).also { shader ->
            GLES30.glShaderSource(shader, code)
            GLES30.glCompileShader(shader)
            val compiled = IntArray(1)
            GLES30.glGetShaderiv(shader, GLES30.GL_COMPILE_STATUS, compiled, 0)
            if (compiled[0] == 0) {
                val log = GLES30.glGetShaderInfoLog(shader)
                GLES30.glDeleteShader(shader)
                throw RuntimeException("Shader compile error: $log")
            }
        }

    private fun createFloatBuffer(data: FloatArray): FloatBuffer =
        ByteBuffer.allocateDirect(data.size * 4).order(ByteOrder.nativeOrder()).asFloatBuffer().apply {
            put(data).position(0)
        }

    fun setFocusModeEnabled(enabled: Boolean) {
        focusModeEnabled = enabled
    }

    fun setZebraPatternEnabled(enabled: Boolean) {
        zebraPatternEnabled = enabled
    }

    fun updateSurfaceSize(width: Int, height: Int) {
        surfaceWidth = width
        surfaceHeight = height

        cameraTexture?.setDefaultBufferSize(width, height)
    }
}