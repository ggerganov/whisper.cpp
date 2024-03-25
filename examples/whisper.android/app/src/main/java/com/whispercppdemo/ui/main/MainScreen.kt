package com.whispercppdemo.ui.main

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable

import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.runtime.livedata.observeAsState
import androidx.compose.ui.Alignment

import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.isGranted
import com.google.accompanist.permissions.rememberPermissionState
import com.whispercppdemo.R


@Composable
fun MainScreen(viewModel: MainScreenViewModel) {

    val canTranscribeState = viewModel.canTranscribe.observeAsState(initial = false)
    val isRecordingState = viewModel.isRecording.observeAsState(initial = false)
    val isStreamingState = viewModel.isStreaming.observeAsState(initial = false)


    val messageLogState = viewModel.dataLog.observeAsState(initial = "")
    val processingTimeMessage = viewModel.processingTimeMessage.observeAsState(initial = "")

    MainScreen(
        canTranscribe = canTranscribeState.value,
        isRecording = isRecordingState.value,
        isStreaming = isStreamingState.value,
        messageLog = messageLogState.value,
        processingTimeMessage = processingTimeMessage.value,
        onBenchmarkTapped = viewModel::benchmark,
        onTranscribeSampleTapped = viewModel::transcribeSample,
        onRecordTapped = viewModel::toggleRecord,
        onStreamTapped = viewModel::toggleStream
    )

}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    canTranscribe: Boolean,
    isRecording: Boolean,
    isStreaming: Boolean,
    messageLog: String,
    processingTimeMessage: String,
    onBenchmarkTapped: () -> Unit,
    onTranscribeSampleTapped: () -> Unit,
    onRecordTapped: () -> Unit,
    onStreamTapped: () -> Unit
) {
    val scrollState = rememberScrollState()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(stringResource(id = R.string.app_name)) }
            )
        },
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .padding(innerPadding)
                .padding(16.dp)
        ) {
            Column(verticalArrangement = Arrangement.SpaceBetween) {
                Row(horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth()) {
                    BenchmarkButton(enabled = canTranscribe, onClick = onBenchmarkTapped)
                    TranscribeSampleButton(enabled = canTranscribe, onClick = onTranscribeSampleTapped)
                }
                RecordSection(
                    enabled = canTranscribe,
                    isRecording = isRecording,
                    processingTimeMessage = processingTimeMessage,
                    onClick = onRecordTapped
                )
                StreamButton(
                    enabled = canTranscribe,
                    isStreaming = isStreaming,
                    onClick = onStreamTapped
                )
            }
            MessageLog(messageLog)
        }
    }
}


@Composable
fun MessageLog(log: String) {
    SelectionContainer {
        Text(modifier = Modifier.verticalScroll(rememberScrollState()), text = log)
    }
}

@Composable
fun BenchmarkButton(enabled: Boolean, onClick: () -> Unit) {
    Button(onClick = onClick, enabled = enabled) {
        Text("Benchmark")
    }
}

@Composable
fun TranscribeSampleButton(enabled: Boolean, onClick: () -> Unit) {
    Button(onClick = onClick, enabled = enabled) {
        Text("Transcribe sample")
    }
}

@OptIn(ExperimentalPermissionsApi::class)
@Composable
fun RecordButton(enabled: Boolean, isRecording: Boolean, onClick: () -> Unit) {
    val micPermissionState = rememberPermissionState(
        permission = android.Manifest.permission.RECORD_AUDIO,
        onPermissionResult = { granted ->
            if (granted) {
                onClick()
            }
        }
    )
    Button(onClick = {
        if (micPermissionState.status.isGranted) {
            onClick()
        } else {
            micPermissionState.launchPermissionRequest()
        }
    }, enabled = enabled) {
        Text(
            if (isRecording) {
                "Stop recording"
            } else {
                "Start recording"
            }
        )
    }
}
@Composable
fun RecordSection(enabled: Boolean, isRecording: Boolean, processingTimeMessage: String, onClick: () -> Unit) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier.fillMaxWidth()
    ) {
        RecordButton(
            enabled = enabled,
            isRecording = isRecording,
            onClick = onClick
        )
        Spacer(Modifier.width(8.dp))
        Text(text = processingTimeMessage)
    }
}
@OptIn(ExperimentalPermissionsApi::class)
@Composable
fun StreamButton(enabled: Boolean, isStreaming: Boolean, onClick: () -> Unit) {
    val micPermissionState = rememberPermissionState(
        permission = android.Manifest.permission.RECORD_AUDIO,
        onPermissionResult = { granted ->
            if (granted) {
                onClick()
            }
        }
    )
    Button(onClick = {
        if (micPermissionState.status.isGranted) {
            onClick()
        } else {
            micPermissionState.launchPermissionRequest()
        }
    }, enabled = enabled) {
        Text(
            if (isStreaming) {
                "Stop streaming"
            } else {
                "Start streaming"
            }
        )
    }
}