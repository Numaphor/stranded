# 🎮 GBAjs3 Testing Guide for NPC Merchant Z-Order Fix

## 🚀 Quick Start Testing

### Step 1: Upload ROM to GBAjs3
1. **Visit**: https://gba.nicholas-vancise.dev
2. **Click**: "Upload Rom" in the menu
3. **Select**: `stranded.gba` from your file system
4. **Wait**: ROM loads automatically in the browser

### Step 2: Game Controls
**Default GBAjs3 Controls**:
- **Arrow Keys**: Movement (Up, Down, Left, Right)
- **Z Key**: A button (Action/Confirm)
- **X Key**: B button (Cancel/Back)
- **A Key**: L shoulder button
- **S Key**: R shoulder button
- **Enter**: Start button
- **Shift**: Select button

**Alternative**: Click the virtual controls on screen (mobile-friendly)

### Step 3: Navigate to Merchant NPC
1. **Start Game**: Use Z key (A button) to start
2. **Move Player**: Use arrow keys to navigate
3. **Find Merchant**: Located at position (100, -50) in the world
4. **Approach**: Walk near the merchant NPC

### Step 4: Test Z-Order Fix
**Critical Test Scenarios**:

#### Scenario A: Player Above Merchant
1. **Position**: Move player to Y position above merchant (lower Y value)
2. **Expected**: Player appears **behind** merchant (merchant in front)
3. **Verify**: Player sprite should be partially or fully obscured by merchant

#### Scenario B: Player Below Merchant  
1. **Position**: Move player to Y position below merchant (higher Y value)
2. **Expected**: Player appears **in front** of merchant (player in front)
3. **Verify**: Player sprite should be visible over merchant sprite

### Step 5: Debug Verification
1. **Open Console**: Press F12 in browser
2. **Check Output**: Look for debug messages like:
   ```
   Merchant z-order: -7 (pos.y: -50)
   Player z-order: -8 (when above merchant)
   Player z-order: -6 (when below merchant)
   ```

## 🔍 What to Look For

### ✅ Success Indicators
- **Proper Layering**: Player correctly appears behind/in front based on Y position
- **Smooth Movement**: No visual glitches during depth sorting
- **Debug Output**: Console shows dynamic z-order calculations
- **Consistent Behavior**: Depth sorting works reliably in all positions

### ❌ Failure Indicators
- **Static Layering**: Merchant always appears in same layer regardless of position
- **Visual Glitches**: Sprites flickering or appearing incorrectly
- **Missing Debug**: No console output showing z-order calculations
- **Inconsistent Behavior**: Depth sorting works sometimes but not always

## 🎯 Expected Behavior

### Before Fix (Broken)
- Merchant had static z-order of `2`
- Player z-order range: -10 to -5
- Result: Merchant always appeared incorrectly layered

### After Fix (Working)
- Merchant z-order: `(_pos.y() / 8) - 1 = -7` (for Y position -50)
- Player z-order range: -10 to -5
- Result: Proper depth sorting based on Y position

## 🛠️ Troubleshooting

### If ROM Won't Load
- **Check File**: Ensure `stranded.gba` is valid (1.4MB, GBA ROM format)
- **Browser**: Try different browser (Chrome, Firefox, Safari)
- **Clear Cache**: Refresh page or clear browser cache

### If Controls Don't Work
- **Focus**: Click on game area to ensure it has focus
- **Remap**: Use GBAjs3 "Controls" menu to customize key bindings
- **Virtual**: Try on-screen virtual controls instead

### If Z-Order Seems Wrong
- **Position**: Ensure you're testing at the exact merchant location
- **Movement**: Try moving gradually to see transition points
- **Console**: Check F12 console for debug output
- **Reload**: Try "Quick Reload" in GBAjs3 menu

## 📱 Mobile Testing
- **Touch Controls**: Use virtual D-pad and buttons
- **Responsive**: Interface adapts to mobile screens
- **Performance**: May be slower than desktop but fully functional

## 🎉 Success Confirmation
When working correctly, you should see:
1. **Dynamic Layering**: Player depth changes based on position relative to merchant
2. **Debug Output**: Console shows calculated z-order values
3. **Smooth Gameplay**: No visual artifacts or glitches
4. **Consistent Behavior**: Works reliably across multiple tests

---

**Ready to test!** Upload `stranded.gba` to https://gba.nicholas-vancise.dev and verify the NPC Merchant z-order fix is working perfectly! 🚀
