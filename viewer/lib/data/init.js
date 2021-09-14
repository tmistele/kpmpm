
loadScript('qrc:///qtwebchannel/qwebchannel.js').then(() => {
    new QWebChannel(qt.webChannelTransport, (channel) => {
        const sourceTextObject = channel.objects.sourceTextObject;
        const viewObject = channel.objects.viewObject;
        const origScrollToFirstChange = scrollToFirstChange;

        viewObject.requestReloadImagesWithoutCache.connect(() => {

            showStatusInfo('Reloading images...');

            const imgReloadPromises = [];

            const images = container.querySelectorAll('img');
            for(const img of images) {
                const iframe = document.createElement('iframe');
                iframe.onload = () => {

                    let _resolve;
                    const promise = new Promise((resolve, reject) => {
                        _resolve = resolve;
                    });
                    imgReloadPromises.push(promise);

                    const doc = iframe.contentWindow.document;
                    const tmpimg = doc.createElement('img');
                    tmpimg.onload = () => {
                        img.src = img.src;
                        document.body.removeChild(iframe);
                        _resolve();
                    }
                    // Do the same on error. Important for handling non-existing / deleted images
                    tmpimg.onerror = tmpimg.onload;
                    tmpimg.src = img.src;
                    doc.body.appendChild(tmpimg);

                    // The above seems to work even without iframe reload.
                    // If this stops working at some point, we can explicitly reload the iframe without cache
                    // iframe.contentWindow.location.reload(true);
                };
                iframe.style.display = 'none';
                document.body.appendChild(iframe);
            }

            Promise.all(imgReloadPromises).then(() => {
                hideStatus();
            });
        });

        window.onscroll = function() {
            viewObject.setScrollPosition(window.scrollX, window.scrollY);
        };

        viewObject.requestSetScrollPosition.connect((x, y) => {
            window.scrollTo(x, y);
        });

        sourceTextObject.urlChanged.connect((url) => {
            // Prevent scroll/highlight of first change when loading a new document
            // This is useless because
            // - first change will always be first block
            // - we scroll to previous scroll position anyway in this case (requestSetScrollPosition)
            scrollToFirstChange = () => {
                scrollToFirstChange = origScrollToFirstChange;
            };
        });

        // Tell C++ whenever rendering of a new content finishes
        document.addEventListener('pmpmRenderingDone', (_) => {
            viewObject.emitRenderingDone();
        });

        // Tell C++ code that we are ready to take content
        getWebsocket().then(() => {
            viewObject.emitInitDone();
        });

        viewObject.reconnectWithNewSecret.connect((newSecret) => {
            initWebsocketWithNewSecret(newSecret);
        });

        // Tell C++ whenever auth fails (usually after pmpm restart)
        document.addEventListener('pmpmAuthFail', (_) => {
            viewObject.emitAuthFail();
        });

        // When init.js loads slower than auth fails, we miss the initial
        // pmpmAuthFail event. To catch this, explicitly check the auth status
        if(false === getWebsocketAuthStatus())
            viewObject.emitAuthFail();
    });
});
