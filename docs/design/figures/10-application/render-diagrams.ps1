# Render the rectangles, text and arrows emitted by build-diagrams.py with Windows WPF.
# Offline; does not open a browser or require the product toolchains.
param(
  [ValidateSet('01-component-call-map','02-firmware-call-map','03-default-off-and-probe')]
  [string[]]$DiagramNames = @('01-component-call-map','02-firmware-call-map','03-default-off-and-probe')
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName WindowsBase,PresentationCore

function Get-Brush([string]$color) {
  if ($color -eq 'none' -or $color -eq '') { return $null }
  return [Windows.Media.SolidColorBrush]::new(([Windows.Media.ColorConverter]::ConvertFromString($color)))
}

function Get-Pen([string]$color, [double]$width) {
  $brush = Get-Brush $color
  if ($brush) { return [Windows.Media.Pen]::new($brush, $width) }
  return $null
}

function Add-Polygon($drawing, $points, $fill, $pen, [bool]$closed) {
  $geometry = [Windows.Media.StreamGeometry]::new()
  $context = $geometry.Open()
  try {
    $context.BeginFigure($points[0], $closed, $closed)
    foreach ($point in ($points | Select-Object -Skip 1)) { $context.LineTo($point, $true, $false) }
  } finally { $context.Close() }
  $drawing.DrawGeometry($fill, $pen, $geometry)
}

foreach ($diagramName in $DiagramNames) {
  $svgPath = Join-Path $PSScriptRoot ($diagramName + '.svg')
  [xml]$document = [System.IO.File]::ReadAllText($svgPath)
  $root = $document.DocumentElement
  $visual = [Windows.Media.DrawingVisual]::new()
  $draw = $visual.RenderOpen()
  try {
    foreach ($element in $root.ChildNodes) {
      switch ($element.LocalName) {
        'rect' {
          $box = [Windows.Rect]::new([double]$element.x,[double]$element.y,[double]$element.width,[double]$element.height)
          $draw.DrawRoundedRectangle((Get-Brush $element.fill),(Get-Pen $element.stroke ([double]$element.'stroke-width')),$box,[double]$element.rx,[double]$element.rx)
        }
        'text' {
          $family = $element.'font-family'.Split(',')[0]
          $weight = if ($element.'font-weight' -eq '700') { [Windows.FontWeights]::Bold } else { [Windows.FontWeights]::Normal }
          $type = [Windows.Media.Typeface]::new([Windows.Media.FontFamily]::new($family),[Windows.FontStyles]::Normal,$weight,[Windows.FontStretches]::Normal)
          $formed = [Windows.Media.FormattedText]::new($element.InnerText,[Globalization.CultureInfo]::GetCultureInfo('vi-VN'),[Windows.FlowDirection]::LeftToRight,$type,[double]$element.'font-size',(Get-Brush $element.fill),1.0)
          $originX = [double]$element.x
          if ($element.'text-anchor' -eq 'middle') { $originX -= $formed.Width / 2 }
          $draw.DrawText($formed,[Windows.Point]::new($originX,(([double]$element.y) - $formed.Baseline)))
        }
        'polyline' {
          $points = @($element.points.Split(' ') | ForEach-Object {
            $xy = $_.Split(','); [Windows.Point]::new([double]$xy[0],[double]$xy[1])
          })
          $stroke = Get-Pen $element.stroke ([double]$element.'stroke-width')
          if ($element.GetAttribute('stroke-dasharray')) { $stroke.DashStyle = [Windows.Media.DashStyles]::Dash }
          Add-Polygon $draw $points $null $stroke $false
          $tip = $points[-1]; $previous = $points[-2]
          $dx = $tip.X - $previous.X; $dy = $tip.Y - $previous.Y
          $length = [Math]::Sqrt($dx*$dx + $dy*$dy)
          $ux = $dx/$length; $uy = $dy/$length
          $baseX = $tip.X - 9*$ux; $baseY = $tip.Y - 9*$uy
          $triangle = @($tip,[Windows.Point]::new($baseX-5.4*$uy,$baseY+5.4*$ux),[Windows.Point]::new($baseX+5.4*$uy,$baseY-5.4*$ux))
          Add-Polygon $draw $triangle (Get-Brush $element.stroke) $null $true
        }
      }
    }
  } finally { $draw.Close() }
  $image = [Windows.Media.Imaging.RenderTargetBitmap]::new([int]$root.width,[int]$root.height,96,96,[Windows.Media.PixelFormats]::Pbgra32)
  $image.Render($visual)
  $encoder = [Windows.Media.Imaging.PngBitmapEncoder]::new()
  $encoder.Frames.Add([Windows.Media.Imaging.BitmapFrame]::Create($image))
  $path = Join-Path $PSScriptRoot ($diagramName + '.png')
  $stream = [IO.File]::Create($path)
  try { $encoder.Save($stream) } finally { $stream.Dispose() }
  Get-Item -LiteralPath $path | Select-Object Name,Length
}
