package com.example.temperatueapp

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.ColorMatrix
import android.graphics.ColorMatrixColorFilter
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.util.Log
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.example.temperatueapp.databinding.ActivityMainBinding
import com.google.android.material.snackbar.Snackbar
import info.mqtt.android.service.Ack
import info.mqtt.android.service.MqttAndroidClient
import org.eclipse.paho.client.mqttv3.*
import org.json.JSONObject


class MqttClient(private val context: Context) {

    lateinit var client: MqttAndroidClient
    val clientId = "app2"
    lateinit var snackbarCallback: (input: String) -> Unit

    companion object {
        const val TAG = "MqttClient"
    }

    fun connect(broker: String, username: String, password: String) {
        client =
            MqttAndroidClient(context, broker,clientId, Ack.AUTO_ACK)
        val connOpts: MqttConnectOptions = MqttConnectOptions()
        connOpts.userName = username
        connOpts.password = password.toCharArray()

        try {
            client.connect(connOpts, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken) {
                    Log.i(TAG, "connect succeed")
                }

                override fun onFailure(asyncActionToken: IMqttToken, exception: Throwable) {
                    Log.i(TAG, "connect failed")
                }
            })
        } catch (e: MqttException) {
            e.printStackTrace()
        }

        client.connect(connOpts)
        println("Connected.")
    }

    fun setCallBack(topics: Array<String>? = null,
                    messageCallBack: ((topic: String, message: MqttMessage) -> Unit)? = null,
                    snackbarCallback: (input: String) -> Unit) {
        this.snackbarCallback = snackbarCallback
        client.setCallback(object : MqttCallbackExtended {
            override fun connectComplete(reconnect: Boolean, serverURI: String) {
                topics?.forEach {
                    subscribeTopic(it)
                }
                Log.d(TAG, "Connected to: $serverURI")
            }

            override fun connectionLost(cause: Throwable) {
                Log.d(TAG, "The Connection was lost.")
            }

            @Throws(Exception::class)
            override fun messageArrived(topic: String, message: MqttMessage) {
                Log.d(TAG, "Incoming message from $topic: " + message.toString())
                messageCallBack?.invoke(topic, message)
            }

            override fun deliveryComplete(token: IMqttDeliveryToken) {

            }
        })

    }

    fun publishMessage(topic: String, msg: String) {

        try {
            val message = MqttMessage()
            message.payload = msg.toByteArray()
            client.publish(topic, message.payload, 0, true)
            Log.d(TAG, "$msg published to $topic")
        } catch (e: MqttException) {
            Log.d(TAG, "Error Publishing to $topic: " + e.message)
            e.printStackTrace()
        }

    }

    fun subscribeTopic(topic: String, qos: Int = 0) {
        client.subscribe(topic, qos).actionCallback = object : IMqttActionListener {
            override fun onSuccess(asyncActionToken: IMqttToken) {
                Log.d(TAG, "Subscribed to $topic")
                snackbarCallback("Subscribed!")

            }

            override fun onFailure(asyncActionToken: IMqttToken, exception: Throwable) {
                Log.d(TAG, "Failed to subscribe to $topic")
                exception.printStackTrace()
            }
        }
    }

    fun close() {
        client.apply {
            unregisterResources()
            close()
        }
    }

    fun disconnect() {
        if (client.isConnected)
            client.disconnect()
    }
}

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    lateinit var ct: String

    val user = "97521486"
    val pass = "dF7JpXma"
    val sendingTopic = "97521486/app"
    val recievingTopic = "97521486/system"

    val id = "yasi"
    val serverURI = "tcp://45.149.77.235:1883"

    val mqttClient by lazy {
        MqttClient(this)
    }




    fun changeTextField(value: Int, view: View): Boolean {
        if (value < 15) {
            val snack = Snackbar.make(view,"Too cold!", Snackbar.LENGTH_SHORT)
            snack.show()
            binding.textField.editText?.text = Editable.Factory.getInstance().newEditable("15")
            return false
        }
        else if (value > 40) {
            val snack = Snackbar.make(view,"Too hot!", Snackbar.LENGTH_SHORT)
            snack.show()
            binding.textField.editText?.text = Editable.Factory.getInstance().newEditable("40")
            return false
        }
        binding.textField.editText?.text = Editable.Factory.getInstance().newEditable(value.toString())
        return true
    }


    @SuppressLint("SetTextI18n")
    private fun setData(topic: String, msg: MqttMessage) {
        val matrix = ColorMatrix()
        matrix.setSaturation(0f)

        val data = JSONObject(String(msg.payload))
        val tempC = data.getDouble("tempC")
        val tempF = data.getDouble("tempeF")
        val humidity = data.getInt("humidity")
        val heater = data.getInt("heater")
        val aircon = data.getInt("aircon")
        val motion = data.getInt("motion")

        println(tempC)
        when (topic) {
            topic -> {
                val mainHandler = Handler(Looper.getMainLooper()).post {
                    binding.currTempCText.text="$tempC"
                    binding.currTempFText.text=String.format("%.1f", tempF)
                    binding.currHumidityText.text="$humidity"
                    if (heater == 1) {
                        binding.heaterImg.colorFilter = null
                        binding.currHeaterText.text="On"
                    } else {
                        binding.currHeaterText.text="Off"
                        binding.heaterImg.colorFilter=ColorMatrixColorFilter(matrix)
                    }

                    if (aircon == 1) {
                        binding.airconImg.colorFilter = null
                        binding.currAirconText.text="On"
                    } else {
                        binding.currAirconText.text="Off"
                        binding.airconImg.colorFilter=ColorMatrixColorFilter(matrix)
                    }

                    if (motion == 1) {
                        binding.motionImg.colorFilter=null
                        binding.currMotionText.text="On"
                    } else {
                        binding.currMotionText.text="Off"
                        binding.motionImg.colorFilter = ColorMatrixColorFilter(matrix)
                    }

//                    binding.currAirconText.text= if (aircon == 1) "On" else "Off"
//                    binding.currMotionText.text= if (motion == 1) "Yes" else "No"
                }
            }
            else -> {
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        val view = binding.root
        setContentView(view)


        fun snackbarCallback(input: String): Unit {
            val snack = Snackbar.make(view,input, Snackbar.LENGTH_SHORT)
            snack.show()

        }


        mqttClient.connect(serverURI, user, pass)
        mqttClient.setCallBack(arrayOf(recievingTopic), ::setData, ::snackbarCallback)

        binding.setBtn.setOnClickListener {
            // send req
            if (changeTextField((binding.textField.editText?.text.toString()).toInt(), view))
            {
                mqttClient.publishMessage(sendingTopic, binding.textField.editText?.text.toString())
            }
        }
        binding.addBtn.setOnClickListener {
            changeTextField(binding.textField.editText?.text.toString().toInt() + 1, view)
        }
        binding.subBtn.setOnClickListener {
            changeTextField(binding.textField.editText?.text.toString().toInt() - 1, view)
        }
    }
}