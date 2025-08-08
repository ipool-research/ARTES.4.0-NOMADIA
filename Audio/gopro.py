import asyncio
import aiohttp

class GoPro():
    def __init__(self):
        self.BASE_URL = "http://10.5.5.9:8080/"  # URL base per la GoPro
    
    def start_gopro_video(self):
        return asyncio.run(self._start_gopro_video())

    async def _start_gopro_video(self):
        url = f"{self.BASE_URL}gp/gpControl/command/shutter?p=1"
        async with aiohttp.ClientSession() as session:
            async with session.get(url) as response:
                print("▶️ GoPro avviata:", await response.text())
                return response.status == 200

    def stop_gopro_video(self):
        return asyncio.run(self._stop_gopro_video())

    async def _stop_gopro_video(self):
        url = f"{self.BASE_URL}gp/gpControl/command/shutter?p=0"
        async with aiohttp.ClientSession() as session:
            async with session.get(url) as response:
                print("🛑 GoPro fermata:", await response.text())
                return response.status == 200