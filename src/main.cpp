#include <TFT_eSPI.h>
#include <SPI.h>
#include <AnimatedGIF.h>
#include "spiral.h"  // Your GIF file converted to a header

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 80

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;

// Draw callback
void GIFDraw(GIFDRAW *pDraw)
{
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[320];
    int x, y, iWidth;

    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line
    
    iWidth = pDraw->iWidth;
    if (iWidth > SCREEN_WIDTH)
        iWidth = SCREEN_WIDTH;
        
    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) // restore to background color
    {
        for (x=0; x<iWidth; x++)
        {
            if (s[x] == pDraw->ucTransparent)
                s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
        uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
        int x, iCount;
        pEnd = s + iWidth;
        x = 0;
        iCount = 0; // count non-transparent pixels
        while(x < iWidth)
        {
            c = ucTransparent-1;
            d = usTemp;
            while (c != ucTransparent && s < pEnd)
            {
                c = *s++;
                if (c == ucTransparent) // done, stop
                {
                    s--; // back up to treat it like transparent
                }
                else // opaque
                {
                    *d++ = usPalette[c];
                    iCount++;
                }
            }
            if (iCount) // any opaque pixels?
            {
                tft.setAddrWindow(pDraw->iX+x, y, iCount, 1);
                tft.pushColors(usTemp, iCount, true);
                x += iCount;
                iCount = 0;
            }
            // handle transparent pixels
            while (s < pEnd && *s == ucTransparent)
            {
                s++;
                x++;
            }
        }
    }
    else // no transparency
    {
        s = pDraw->pPixels;
        // Convert palette to RGB565 colors
        for (x=0; x<iWidth; x++)
            usTemp[x] = usPalette[*s++];
        tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
        tft.pushColors(usTemp, iWidth, true);
    }
}

void setup() {
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    
    gif.begin(LITTLE_ENDIAN_PIXELS);
}

void loop() {
    // Open the GIF file from PROGMEM
    if (gif.open((uint8_t *)spiral_gif, sizeof(spiral_gif), GIFDraw)) {
        while (gif.playFrame(true, NULL)) {
            // Wait for next frame
        }
        gif.close();
    }
}
